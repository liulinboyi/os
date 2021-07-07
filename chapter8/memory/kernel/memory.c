#include "memory.h"
#include "bitmap.h"
#include "stdint.h"
#include "global.h"
#include "debug.h"
#include "print.h"
#include "string.h"

#define PG_SIZE 4096 // 用以表示页的尺寸，其值为4096，即4KB。

/**************************************************
32位虚拟地址的转换过程。
（1）高10位是页目录项pde的索引，用于在页目录表中定位pde，细节是处理器获取高10位后自动将其乘以4，再加上页目录表的物理地址，这样便得到了pde索引对应的pde所在的物理地址，然后自动在该物理地址中，即该pde中，获取保存的页表物理地址（为了严谨，说的都有点拗口了）。
（2）中间10位是页表项pte的索引，用于在页表中定位pte。细节是处理器获取中间10位后自动将其乘以4，再加上第一步中得到的页表的物理地址，这样便得到了pte索引对应的pte所在的物理地址，然后自动在该物理地址（该pte）中获取保存的普通物理页的物理地址。
（3）低12位是物理页内的偏移量，页大小是4KB，12位可寻址的范围正好是4KB，因此处理器便直接把低12位作为第二步中获取的物理页的偏移量，无需乘以4。用物理页的物理地址加上这低12位的和便是这32位虚拟地址最终落向的物理地址。
32位地址经过以上三步拆分，地址最终落在某个物理页内。
注意啦，再提醒一次，页表的作用是将虚拟地址转换成物理地址，此工作表面虚幻，但内心真实，其转换过程中涉及访问的页目录表、页目录项及页表项，都是通过真实物理地址访问的，否则若用虚拟地址访问它们的话，会陷入转换的死循环中不可自拔。
***************************************************/

/***************  位图地址 ********************
 * 位图的1字节对等表示8个资源单位,一个资源单位是4k，所以位图的一个字节是32k 1kb => 32Mb
 * "一个字节代表32k，一个页表4k，一个页表代表: 4 * 1024 * 32 = 131072 (k)"
 * "一个页表代表: 131072 / 1024 = 128 (M)"
 * 因为0xc009f000是内核主线程栈顶，0xc009e000是内核主线程的pcb.
 * 一个页框大小的位图可表示128M内存, 位图位置安排在地址0xc009a000,
 * 这样本系统最大支持4个页框的位图,即512M */
 
 // 0xc009e000已经是主线程的PCB，一页大小为0x1000，故再减去4页，即0xc009e000 − 0x4000 = 0xc009a000。故我们的位图地址为0xc009a000。
#define MEM_BITMAP_BASE 0xc009a000 // 内存位图的基址
/*************************************/

// 0xffc00000 == 0b11111111110000000000000000000000
#define PDE_IDX(addr) ((addr & 0xffc00000) >> 22) // 返回虚拟地址的高10位，即pde索引部分，此部分用于在页目录表中定位pde
// 0x003ff000 == 0b00000000001111111111000000000000
#define PTE_IDX(addr) ((addr & 0x003ff000) >> 12) // 返回虚拟地址的中间10位，即pte索引部分，此部分用于在页表中定位pte

// 用来表示内核所使用的堆空间起始虚拟地址，其值为0xc0100000
// 在 loader 中我们已经通过设置页表把虚拟地址 0xc0000000～0xc00fffff 映射到了物理地址 0x00000000～ 0x000fffff（低端 1MB 的内存），故我们为了让虚拟地址连续，将堆的起始虚拟地址设为 0xc0100000。当然，虚拟地址也可以不连续，不过让其连续不是显得紧凑一些吗？这并不是强制的。
/* 0xc0000000是内核从虚拟地址3G起. 0x100000意指跨过低端1M内存,使虚拟地址在逻辑上连续 */
#define K_HEAP_START 0xc0100000

// 内存管理的主角，物理内存池结构体，struct pool，用它来管理本内存池中的所有物理内存
/* 内存池结构,生成两个实例用于管理内核内存池和用户内存池 */
struct pool {
   struct bitmap pool_bitmap;	 // 本内存池用到的位图结构,用于管理物理内存
   uint32_t phy_addr_start;	 // 本内存池所管理物理内存的起始地址
   uint32_t pool_size;		 // 本内存池字节容量
};

struct pool kernel_pool, user_pool;      // 生成内核内存池和用户内存池 们在函数mem_pool_init中被初始化
struct virtual_addr kernel_vaddr;	 // 此结构是用来给内核分配虚拟地址

/* 在pf表示的(内核、用户)虚拟内存池中申请pg_cnt个虚拟页,
 * 成功则返回虚拟页的起始地址, 失败则返回NULL */
static void* vaddr_get(enum pool_flags pf, uint32_t pg_cnt) {
   int vaddr_start = 0/*用于存储分配的起始虚拟地址*/, bit_idx_start = -1; /*用于存储位图扫描函数bitmap_scan的返回值，默认为−1*/
   uint32_t cnt = 0;
   if (pf == PF_KERNEL) { // 在内核虚拟地址池中申请地址，于是调用bitmap_scan函数扫描内核虚拟地址池中的位图。
      bit_idx_start  = bitmap_scan(&kernel_vaddr.vaddr_bitmap, pg_cnt);
      if (bit_idx_start == -1) { // 若bitmap_scan返回−1，则vaddr_get函数返回NULL
	 return NULL;
      }
      while(cnt < pg_cnt) { // 根据申请的页数量，即pg_cnt的值，逐次调用bitmap_set函数将相应位置1
	 bitmap_set(&kernel_vaddr.vaddr_bitmap, bit_idx_start + cnt++, 1);
      }
      vaddr_start = kernel_vaddr.vaddr_start + bit_idx_start * PG_SIZE; // 将 bit_idx_start 转换为虚拟地址
   } else {	
   // 用户内存池,将来实现用户进程再补充
   }
   return (void*)vaddr_start; // 将vaddr_star转换成指针后返回
}

/* 得到虚拟地址vaddr对应的pte(页表项)指针*/
// 功能是得到地址vaddr所在pte(页表项)的指针，强调下，指针的值也就是虚拟地址，故此函数实际返回的是能够访问vaddr所在pte的虚拟地址。
/*
处理器处理32位地址的三个步骤如下。
（1）首先处理高10位的pde索引，从而处理器得到页表物理地址。
（2）其次处理中间10位的pte索引，进而处理器得到普通物理页的物理地址。
（3）最后是把低12位作为普通物理页的页内偏移地址，此偏移地址加上物理页的物理地址，得到的地址之和便是最终的物理地址，处理器到此物理地址上进行读写操作。
也就是说，我们要创造的这个新的虚拟地址new_vaddr，它经过处理器以上三个步骤的拆分处理，最终会落到vaddr自身所在的pte的物理地址上。
pte位于页表中，因此要想访问vaddr所在的pte，必须保证处理器在第2步处理pte索引时得到的是页表的物理地址，而不是普通物理页的物理地址，这样可以再利用第3步中的低12位做页表内的偏移量，用此偏移量加上页表物理地址，所得的地址之和便是vaddr所在的pte的物理地址。
*/
uint32_t* pte_ptr(uint32_t vaddr) {
   /* 先访问到页表自己 + \
    * 再用页目录项pde(页目录内页表的索引)做为pte的索引访问到页表 + \
    * 再用pte的索引做为页内偏移*/
   uint32_t* pte = (uint32_t*)(0xffc00000 + \
	 ((vaddr & 0xffc00000) >> 10) + \
	 PTE_IDX(vaddr) * 4);
	 int value = (int) pte;
	 put_str("pte:");
	 put_int(value);
	 put_str("\n");
   return pte;
}

/* 得到虚拟地址vaddr对应的pde的指针 */
/*
由于要访问的是vaddr所在的页目录项pde，所以必须想办法在第2步中让处理器处理pte索引时获得的是页目录表物理地址，然后利用低12位作为物理页的偏移量，此偏移量加上页目录表的物理地址，所得的地址之和便是vaddr所在的pde的物理地址。
其实早在long long ago介绍页表的时候就和大伙儿说过了，由于最后一个页目录项中存储的是页目录表物理地址，故当32位地址中高20位为0xfffff时，这就表示访问到的是最后一个页目录项，即获得了页目录表物理地址。
*/
uint32_t* pde_ptr(uint32_t vaddr) {
   /* 0xfffff是用来访问到页表本身所在的地址 */
   uint32_t* pde = (uint32_t*)((0xfffff000) + PDE_IDX(vaddr) * 4);
   int value = (int) pde;
   put_str("pde:");
   put_int(value);
   put_str("\n");
   return pde;
}

/* 在m_pool指向的物理内存池中分配1个物理页,
 * 成功则返回页框的物理地址,失败则返回NULL */
static void* palloc(struct pool* m_pool) {
   /* 扫描或设置位图要保证原子操作 */
   int bit_idx = bitmap_scan(&m_pool->pool_bitmap, 1);    // 找一个物理页面
   if (bit_idx == -1 ) {
      return NULL;
   }
   bitmap_set(&m_pool->pool_bitmap, bit_idx, 1);	// 将此位bit_idx置1
   uint32_t page_phyaddr = ((bit_idx * PG_SIZE) + m_pool->phy_addr_start);
   return (void*)page_phyaddr;
}

/* 页表中添加虚拟地址_vaddr与物理地址_page_phyaddr的映射 */
// 功能是添加虚拟地址_vaddr与物理地址_page_phyaddr的映射。
// 虚拟地址和物理地址的映射关系是在页表中完成的，本质上是在页表中添加此虚拟地址对应的页表项pte，并把物理页的物理地址写入此页表项pte中。
static void page_table_add(void* _vaddr, void* _page_phyaddr) {
   uint32_t vaddr = (uint32_t)_vaddr, page_phyaddr = (uint32_t)_page_phyaddr;
   uint32_t* pde = pde_ptr(vaddr);
   uint32_t* pte = pte_ptr(vaddr);

/************************   注意   *************************
 * 执行*pte,会访问到空的pde。所以确保pde创建完成后才能执行*pte,
 * 否则会引发page_fault。因此在*pde为0时,*pte只能出现在下面else语句块中的*pde后面。
 * *********************************************************/
   /* 先在页目录内判断目录项的P位，若为1,则表示该表已存在 */
   if (*pde & 0x00000001) { // pde存在	 // 页目录项和页表项的第0位为P,此处判断目录项是否存在
      ASSERT(!(*pte & 0x00000001)); // 按理说申请新的地址时其所对应的pte不会存在,如果存在会报错

      if (!(*pte & 0x00000001)) { // 新的地址时其所对应的pte不存在   // 只要是创建页表,pte就应该不存在,多判断一下放心
	 *pte = (page_phyaddr | PG_US_U | PG_RW_W | PG_P_1);    // US=1,RW=1,P=1
      } else {			    //应该不会执行到这，因为上面的ASSERT会先执行。
	 PANIC("pte repeat");
	 *pte = (page_phyaddr | PG_US_U | PG_RW_W | PG_P_1);      // US=1,RW=1,P=1
      }
   } else {			    // 页目录项不存在,所以要先创建页目录再创建页表项.
      /* 页表中用到的页框一律从内核空间分配 */
      uint32_t pde_phyaddr = (uint32_t)palloc(&kernel_pool);

      *pde = (pde_phyaddr | PG_US_U | PG_RW_W | PG_P_1);

      /* 分配到的物理页地址pde_phyaddr对应的物理内存清0,
       * 避免里面的陈旧数据变成了页表项,从而让页表混乱.
       * 访问到pde对应的物理地址,用pte取高20位便可.
       * 因为pte是基于该pde对应的物理地址内再寻址,
       * 把低12位置0便是该pde对应的物理页的起始*/
      memset((void*)((int)pte & 0xfffff000), 0, PG_SIZE);
         
      ASSERT(!(*pte & 0x00000001));
      *pte = (page_phyaddr | PG_US_U | PG_RW_W | PG_P_1);      // US=1,RW=1,P=1
   }
}

/* 分配pg_cnt个页空间,成功则返回起始虚拟地址,失败时返回NULL */
void* malloc_page(enum pool_flags pf, uint32_t pg_cnt) {
/*
监督申请的内存页数pg_cnt是否超过了物理内存池的容量。内核和用户空间各约16MB空间，保守起见用15MB来限制，申请的内存页数要小于内存池大小，即pg_cnt<15*1024*1024/4096 = 3840页。
*/
   ASSERT(pg_cnt > 0 && pg_cnt < 3840);
/***********   malloc_page的原理是三个动作的合成:   ***********
      1通过vaddr_get在虚拟内存池中申请虚拟地址
      2通过palloc在物理内存池中申请物理页
      3通过page_table_add将以上得到的虚拟地址和物理地址在页表中完成映射
***************************************************************/
   void* vaddr_start = vaddr_get(pf, pg_cnt);
   if (vaddr_start == NULL) {
      return NULL;
   }

   uint32_t vaddr = (uint32_t)vaddr_start, cnt = pg_cnt;
   struct pool* mem_pool = pf & PF_KERNEL ? &kernel_pool : &user_pool;

   /* 因为虚拟地址是连续的,但物理地址可以是不连续的,所以逐个做映射*/
   while (cnt-- > 0) {
      void* page_phyaddr = palloc(mem_pool);
      if (page_phyaddr == NULL) {  // 失败时要将曾经已申请的虚拟地址和物理页全部回滚，在将来完成内存回收时再补充
	 return NULL;
      }
      // 若物理页申请成功，再调用“page_table_add((void*)vaddr, page_phyaddr)”将虚拟地址vaddr映射为物理地址page_phyaddr。随后通过“vaddr += PG_SIZE”将vaddr更新为下一个虚拟页，继续下一个循环的申请物理页和页表映射。
      page_table_add((void*)vaddr, page_phyaddr); // 在页表中做映射 
      vaddr += PG_SIZE;		 // 下一个虚拟页
   }
   return vaddr_start;
}

/* 从内核物理内存池中申请pg_cnt页内存,成功则返回其虚拟地址,失败则返回NULL */
void* get_kernel_pages(uint32_t pg_cnt) {
   void* vaddr =  malloc_page(PF_KERNEL, pg_cnt);
   if (vaddr != NULL) {	   // 若分配的地址不为空,将页框清0后返回
      memset(vaddr, 0, pg_cnt * PG_SIZE);
   }
   return vaddr;
}

/* 初始化内存池 */
// 函数的功能是根据内存容量all_mem的大小初始化物理内存池的相关结构
static void mem_pool_init(uint32_t all_mem/*内存容量值 0x02000000*/) {
   put_str("   mem_pool_init start\n");
   uint32_t page_table_size /*记录页目录表和页表占用的字节大小，总大小等于页目录表大小+页表大小。*/ = PG_SIZE * 256; // 0x100000	  // 页表大小= 1页的页目录表+第0和第768个页目录项指向同一个页表+
                                                  // 第769~1022个页目录项共指向254个页表,共256个页框
   uint32_t used_mem /*当前已使用的内存字节数*/ = page_table_size + 0x100000; // 0x200000	  // 0x100000为低端1M内存
   uint32_t free_mem /*0x1e00000*/ /*目前可用的内存字节数*/ = all_mem /*0x02000000*/ - used_mem /*0x200000*/; // 总内存all_mem减去used_mem便是free_mem
   uint16_t all_free_pages /*7680*/ /*可用内存字节数free_mem转换成的物理页数*/ = free_mem / PG_SIZE /*4096*/;		  // 1页为4k,不管总内存是不是4k的倍数,
								  // 对于以页为单位的内存分配策略，不足1页的内存不用考虑了。
   uint16_t kernel_free_pages /*3840*/ = all_free_pages /*7680*/ / 2; // 分配给内核的空闲物理页
   uint16_t user_free_pages /*3840*/ = all_free_pages /*7680*/ - kernel_free_pages /*3840*/; // 把分配给内核后剩余的空闲物理页作为用户内存池的空闲物理页数量

/* 为简化位图操作，余数不处理，坏处是这样做会丢内存。
好处是不用做内存的越界检查,因为位图表示的内存少于实际物理内存*/
   uint32_t kbm_length /*480*/ = kernel_free_pages /*3840*/ / 8;			  // Kernel BitMap(位图)的长度,位图中的一位表示一页,以字节为单位,所以一个位图可以表示8页
   uint32_t ubm_length /*480*/ = user_free_pages / 8;			  // User BitMap的长度. 同上

   uint32_t kp_start = used_mem; // 0x200000				  // Kernel Pool start,内核内存池的起始地址
   uint32_t up_start /*0x1100000*/ = kp_start /*0x200000*/ + kernel_free_pages /*3840*/ * PG_SIZE/*4096*/;	  // User Pool start,用户内存池的起始地址 用户物理内存池的起始地址，其值等于kp_start加上内核物理内存池中的内存字节数

   kernel_pool.phy_addr_start /*0x200000*/ = kp_start; // 内核内存池的起始地址
   user_pool.phy_addr_start /*0x1100000*/   = up_start; // 用户内存池的起始地址

   kernel_pool.pool_size /*0xf00000*/ = kernel_free_pages /*3840*/ * PG_SIZE /*4096*/; // 内核内存池的大小
   user_pool.pool_size /*0xf00000*/ = user_free_pages /*3840*/ * PG_SIZE /*4096*/; // 用户内存池的大小

   kernel_pool.pool_bitmap.btmp_bytes_len /*480*/ = kbm_length; // 内核内存池的位图中的位图字节长度
   user_pool.pool_bitmap.btmp_bytes_len /*480*/ = ubm_length; // 用户存池的位图中的位图字节长度

/*********    内核内存池和用户内存池位图   ***********
 *   位图是全局的数据，长度不固定。
 *   全局或静态的数组需要在编译时知道其长度，
 *   而我们需要根据总内存大小算出需要多少字节。
 *   所以改为指定一块内存来生成位图.
 *   ************************************************/
// 内核使用的最高地址是0xc009f000,这是主线程的栈地址.(内核的大小预计为70K左右)
// 内核内存池的位图先定在MEM_BITMAP_BASE(0xc009a000)处.
   kernel_pool.pool_bitmap.bits /*0xc009a000*/ = (void*)MEM_BITMAP_BASE;
							       
/* 用户内存池的位图紧跟在内核内存池位图之后 */
   user_pool.pool_bitmap.bits /*0xc009a1e0*/ = (void*)(MEM_BITMAP_BASE /*0xc009a000*/ + kbm_length /*480*/);
   /******************** 输出内存池信息 **********************/
   // 内存池的所用位图的起始地址和内存池的起始物理地址
   put_str("      kernel_pool_bitmap_start:");put_int((int)kernel_pool.pool_bitmap.bits);
   put_str(" kernel_pool_phy_addr_start:");put_int(kernel_pool.phy_addr_start);
   put_str("\n");
   put_str("      user_pool_bitmap_start:");put_int((int)user_pool.pool_bitmap.bits);
   put_str(" user_pool_phy_addr_start:");put_int(user_pool.phy_addr_start);
   put_str("\n");

   /* 将位图置0*/
   bitmap_init(&kernel_pool.pool_bitmap);
   bitmap_init(&user_pool.pool_bitmap);

   /* 下面初始化内核虚拟地址的位图,按实际物理内存大小生成数组。*/
   kernel_vaddr.vaddr_bitmap.btmp_bytes_len = kbm_length;      // 用于维护内核堆的虚拟地址,所以要和内核内存池大小一致

  /* 位图的数组指向一块未使用的内存,目前定位在内核内存池和用户内存池之外*/
  // 安排在紧挨着内核内存池和用户内存池所用的位图之后
   kernel_vaddr.vaddr_bitmap.bits = (void*)(MEM_BITMAP_BASE + kbm_length + ubm_length);

   kernel_vaddr.vaddr_start = K_HEAP_START;
   /* 将位图置0*/
   bitmap_init(&kernel_vaddr.vaddr_bitmap); // 调用bitmap_init将其位图初始化
   put_str("   mem_pool_init done\n");
}

/* 内存管理部分初始化入口 */
void mem_init() {
   put_str("mem_init start\n");
   // // 先把物理地址0xb00转换成32位整型指针，再通过* 对该指针做取值操作，这样就获取到了内存容量
   // uint32_t mem_bytes_total = (*(uint32_t*)(0xb00)); // 内存容量保存在汇编变量total_mem_bytes中，其物理地址为0xb00 boches: x 0xb00
   // put_int((uint32_t*)(0xb00));
   // put_str("\n");
   // 0xc0000b00
   uint32_t mem_bytes_total = (*(uint32_t*)(0xc0000b00)); // 先把虚拟地址0xc0000b00转换成32位整型指针，再通过* 对该指针做取值操作，这样就获取到了内存容量
   put_str("mem_bytes_total:");
   put_int(mem_bytes_total);
   put_str("\n");
   mem_pool_init(mem_bytes_total);	  // 初始化内存池
   put_str("mem_init done\n");
}
