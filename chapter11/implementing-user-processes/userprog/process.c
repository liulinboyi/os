#include "process.h"
#include "global.h"
#include "debug.h"
#include "memory.h"
#include "thread.h"    
#include "list.h"    
#include "tss.h"    
#include "interrupt.h"
#include "string.h"
#include "console.h"

extern void intr_exit(void); // 声明了外部函数intr_exit，这是用户进程进入3特权级的关键。

/* 构建用户进程初始上下文信息 */
void start_process(void* filename_) {
   void* function = filename_;
   struct task_struct* cur = running_thread();
   cur->self_kstack += sizeof(struct thread_stack); // 指针self_kstack跨过struct thread_stack栈，最终指向struct intr_stack栈的最低处
   struct intr_stack* proc_stack = (struct intr_stack*)cur->self_kstack; // 指向 self_kstack，也就是struct intr_stack栈的最低处	 
   proc_stack->edi = proc_stack->esi = proc_stack->ebp = proc_stack->esp_dummy = 0; // 对栈中8个通用寄存器初始化
   proc_stack->ebx = proc_stack->edx = proc_stack->ecx = proc_stack->eax = 0; // 对栈中8个通用寄存器初始化
   proc_stack->gs = 0;		 // 用户态用不上,直接初始为0 操作系统不允许用户进程访问显存，所以将其初始化为0
   proc_stack->ds = proc_stack->es = proc_stack->fs = SELECTOR_U_DATA;
   proc_stack->eip = function;	 // 待执行的用户程序地址
   proc_stack->cs = SELECTOR_U_CODE; // 栈中代码段寄存器cs赋值为先前我们已在GDT中安装好的用户级代码段
   proc_stack->eflags = (EFLAGS_IOPL_0 | EFLAGS_MBS | EFLAGS_IF_1);
   // 先获取特权级3的栈的下边界地址，将此地址再加上PG_SIZE，所得的和就是栈的上边界，即栈底
   proc_stack->esp = (void*)((uint32_t)get_a_page(PF_USER, USER_STACK3_VADDR) + PG_SIZE) ;
   proc_stack->ss = SELECTOR_U_DATA; 
   asm volatile ("movl %0, %%esp; jmp intr_exit" : : "g" (proc_stack) : "memory");
}

/* 激活页表 */
void page_dir_activate(struct task_struct* p_thread) {
/********************************************************
 * 执行此函数时,当前任务可能是线程。
 * 之所以对线程也要重新安装页表, 原因是上一次被调度的可能是进程,
 * 否则不恢复页表的话,线程就会使用进程的页表了。
 ********************************************************/

/* 若为内核线程,需要重新填充页表为0x100000 */
   uint32_t pagedir_phy_addr = 0x100000;  // 默认为内核的页目录物理地址,也就是内核线程所用的页目录表
   if (p_thread->pgdir != NULL)	{    // 用户态进程有自己的页目录表
      pagedir_phy_addr = addr_v2p((uint32_t)p_thread->pgdir); // p_thread->pgdir是虚拟地址，需要转换为物理地址
   }

   /* 更新页目录寄存器cr3,使新页表生效 */
   asm volatile ("movl %0, %%cr3" : : "r" (pagedir_phy_addr) : "memory");
}

/* 激活线程或进程的页表,更新tss中的esp0为进程的特权级0的栈 */
void process_activate(struct task_struct* p_thread) {
   ASSERT(p_thread != NULL);
   /* 激活该进程或线程的页表 */
   page_dir_activate(p_thread);

   /* 内核线程特权级本身就是0,处理器进入中断时并不会从tss中获取0特权级栈地址,故不需要更新esp0 */
   if (p_thread->pgdir) {
      /* 更新该进程的esp0,用于此进程被中断时保留上下文 */
      update_tss_esp(p_thread);
   }
}

/* 创建页目录表,将当前页表的表示内核空间的pde复制,
 * 成功则返回页目录的虚拟地址,否则返回-1 */
uint32_t* create_page_dir(void) {

   /* 用户进程的页表不能让用户直接访问到,所以在内核空间来申请 */
   uint32_t* page_dir_vaddr = get_kernel_pages(1);
   if (page_dir_vaddr == NULL) {
      console_put_str("create_page_dir: get_kernel_page failed!");
      return NULL;
   }

/************************** 1  先复制页表  *************************************/
   /*  page_dir_vaddr + 0x300*4 是内核页目录的第768项 */
   // 0x300十进制的768 4是每个页目录项的大小，0x300*4表示768个页目录项的偏移量
   // 虚拟地址的高20位为0xfffff，低12位为0x000，即0xfffff000，这也是页目录表中第0个页目录项自身的物理地址
   // 这样内核占用的页目录项被复制到了用户进程的页目录表中，也就是为用户进程创建了访问内核的入口
   memcpy((uint32_t*)((uint32_t)page_dir_vaddr + 0x300*4), (uint32_t*)(0xfffff000+0x300*4), 1024/*256个页目录项一个页目录项是4字节所以这里是256*4=1024*/);
/*****************************************************************************/

/************************** 2  更新页目录地址 **********************************/
   uint32_t new_page_dir_phy_addr = addr_v2p((uint32_t)page_dir_vaddr); // 将虚拟地址转换为vaddr映射的物理地址
   /* 页目录地址是存入在页目录的最后一项,更新页目录地址为新页目录的物理地址 */
   // PG_US_U 4: 0b100
   // PG_RW_W 2: 0b10
   // PG_P_1  1: 0b1
   page_dir_vaddr[1023] = new_page_dir_phy_addr | PG_US_U | PG_RW_W | PG_P_1;
/*****************************************************************************/
   return page_dir_vaddr;
}

/* 创建用户进程虚拟地址位图 */
void create_user_vaddr_bitmap(struct task_struct* user_prog/*用户进程*/) {
   // 位图中所管理的内存空间的起始地址，我们为用户进程定的起始地址 值为0x8048000
   // 这是Linux用户程序入口地址，您可以用readelf命令查看一下，大部分可执行程序的“Entry point address”都是在0x8048000附近
   user_prog->userprog_vaddr.vaddr_start = USER_VADDR_START;
   // 除法的向上取整DIV_ROUND_UP
   uint32_t bitmap_pg_cnt = DIV_ROUND_UP((0xc0000000 - USER_VADDR_START) / PG_SIZE / 8 , PG_SIZE);
   user_prog->userprog_vaddr.vaddr_bitmap.bits = get_kernel_pages(bitmap_pg_cnt);
   user_prog->userprog_vaddr.vaddr_bitmap.btmp_bytes_len = (0xc0000000 - USER_VADDR_START) / PG_SIZE / 8;
   bitmap_init(&user_prog->userprog_vaddr.vaddr_bitmap);
}

/* 创建用户进程 */
void process_execute(void* filename/*用户进程地址*/, char* name/*进程名*/) { 
   /* pcb内核的数据结构,由内核来维护进程信息,因此要在内核内存池中申请 */
   struct task_struct* thread = get_kernel_pages(1);
   init_thread(thread, name, default_prio); 
   create_user_vaddr_bitmap(thread);
   thread_create(thread, start_process, filename);
   thread->pgdir = create_page_dir();
   
   enum intr_status old_status = intr_disable();
   ASSERT(!elem_find(&thread_ready_list, &thread->general_tag));
   list_append(&thread_ready_list, &thread->general_tag);

   ASSERT(!elem_find(&thread_all_list, &thread->all_list_tag));
   list_append(&thread_all_list, &thread->all_list_tag);
   intr_set_status(old_status);
}

