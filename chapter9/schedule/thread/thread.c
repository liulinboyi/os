#include "thread.h"
#include "stdint.h"
#include "string.h"
#include "global.h"
#include "debug.h"
#include "interrupt.h"
#include "print.h"
#include "memory.h"

#define PG_SIZE 4096

struct task_struct* main_thread;    // 主线程PCB
struct list thread_ready_list;	    // 就绪队列
struct list thread_all_list;	    // 所有任务队列
static struct list_elem* thread_tag;// 用于保存队列中的线程结点

extern void switch_to(struct task_struct* cur, struct task_struct* next);

/* 获取当前线程pcb指针 */
struct task_struct* running_thread() {
   uint32_t esp; 
   asm ("mov %%esp, %0" : "=g" (esp));
  /* 取esp整数部分即pcb起始地址 */
   return (struct task_struct*)(esp & 0xfffff000);
}

/* 由kernel_thread去执行function(func_arg) */
static void kernel_thread(thread_func* function, void* func_arg) {
/* 执行function前要开中断,避免后面的时钟中断被屏蔽,而无法调度其它线程 */
   intr_enable();
   function(func_arg); 
}

/* 初始化线程栈thread_stack,将待执行的函数和参数放到thread_stack中相应的位置 */
void thread_create(struct task_struct* pthread, thread_func function, void* func_arg) {
   /* 先预留中断使用栈的空间,可见thread.h中定义的结构 */
   pthread->self_kstack -= sizeof(struct intr_stack);

   /* 再留出线程栈空间,可见thread.h中定义 */
   pthread->self_kstack -= sizeof(struct thread_stack);
   struct thread_stack* kthread_stack = (struct thread_stack*)pthread->self_kstack;
   kthread_stack->eip = kernel_thread;
   kthread_stack->function = function;
   kthread_stack->func_arg = func_arg;
   kthread_stack->ebp = kthread_stack->ebx = kthread_stack->esi = kthread_stack->edi = 0;
}

/* 初始化线程基本信息 */
// 接受3个参数，pthread是待初始化线程的指针，name是线程名称，prio是线程的优先级，此函数功能是将3个参数写入线程的PCB，并且完成PCB一级的其他初始化。
/*
结构体指针，大致可以理解为，这个结构体的地址，起始地址，它的成员依次按照声明排布在这个内存空间中，每个成员也有类型声明，根据类型声明就知道结构体需要多大内存空间了。
*/
void init_thread(struct task_struct* pthread, char* name, int prio) {
   // memset(pthread, 0, sizeof(*pthread)); // 调用memset(pthread, 0, sizeof(*pthread))将pthread所在的PCB清0，即清0一页
   strcpy(pthread->name, name); // 将线程名写入PCB中的name数组中

   if (pthread == main_thread) {
// 为了演示，故直接将status置为TASK_RUNNING，以后再按照正常的逻辑为状态赋值。
/* 由于把main函数也封装成一个线程,并且它一直是运行的,故将其直接设为TASK_RUNNING */
      pthread->status = TASK_RUNNING;
   } else {
      pthread->status = TASK_READY;
   }

/* self_kstack是线程自己在内核态下使用的栈顶地址 */
   pthread->self_kstack = (uint32_t*)((uint32_t)pthread/*本身就是(pcb的起始地址)指针然后转换成uint32_t类型*/ + PG_SIZE/*类型转换后，指针加上一页大小得到一页的最高地址*/); // 线程自己在0特权级下所用的栈，在线程创建之初，它被初始化为线程PCB的最顶端，即(uint32_t)pthread + PG_SIZE。
   pthread->priority = prio; // 优先级
   pthread->ticks = prio;
   pthread->elapsed_ticks = 0; // 表示线程尚未执行过
   pthread->pgdir = NULL; // 线程没有自己的地址空间，因此第65行将线程的页表置空
   /*
   PCB的上端是0特权级栈，将来线程在内核态下的任何栈操作都是用此PCB中的栈，如果出现了某些异常导致入栈操作过多，这会破坏PCB低处的线程信息。为此，需要检测这些线程信息是否被破坏了，stack_magic被安排在线程信息的最边缘，作为它与栈的边缘。目前用不到此值，以后在线程调度时会检测它。pthread->stack_magic自定义个值就行，我这里用的是0x19870916，这与代码功能无关。
   */
   pthread->stack_magic = 0x19870916;	  // 自定义的魔数
}

/* 函数功能是创建一优先级为prio的线程,线程名为name,线程所执行的函数是function(func_arg) */
// 4个参数，name为线程名，prio为线程的优先级，要执行的函数是function，func_arg是函数function的参数
struct task_struct* thread_start(char* name, int prio, thread_func function, void* func_arg) {
/* pcb都位于内核空间,包括用户进程的pcb也是在内核空间 */
// 在内核空间中申请一页内存，即4096字节，将其赋值给新创建的PCB指针thread，即struct task_struct* thread
// 由于get_kernel_page返回的是页的起始地址，故thread指向的是PCB的最低地址
   struct task_struct* thread = get_kernel_pages(1);

   // 3个参数，pthread是待初始化线程的指针，name是线程名称，prio是线程的优先级，此函数功能是将3个参数写入线程的PCB，并且完成PCB一级的其他初始化。
   init_thread(thread, name, prio); // 初始化刚刚创建的thread线程
   thread_create(thread, function, func_arg); //thread_create接受3个参数，pthread是待创建的线程的指针，function是在线程中运行的函数，func_arg是function的参数。函数的功能是初始化线程栈thread_stack，将待执行的函数和参数放到thread_stack中相应的位置。

   // 把新创建的线程加入了就绪队列和全部线程队列
   /* 确保之前不在队列中 */
   ASSERT(!elem_find(&thread_ready_list, &thread->general_tag));
   /* 加入就绪线程队列 */
   list_append(&thread_ready_list, &thread->general_tag);

   /* 确保之前不在队列中 */
   ASSERT(!elem_find(&thread_all_list, &thread->all_list_tag));
   /* 加入全部线程队列 */
   list_append(&thread_all_list, &thread->all_list_tag);

   return thread;
}

/* 将kernel中的main函数完善为主线程 */
static void make_main_thread(void) {
/* 因为main线程早已运行,咱们在loader.S中进入内核时的mov esp,0xc009f000,
就是为其预留了tcb,地址为0xc009e000,因此不需要通过get_kernel_page另分配一页*/
   main_thread = running_thread();
   init_thread(main_thread, "main", 31);

/* main函数是当前线程,当前线程不在thread_ready_list中,
 * 所以只将其加在thread_all_list中. */
   ASSERT(!elem_find(&thread_all_list, &main_thread->all_list_tag));
   list_append(&thread_all_list, &main_thread->all_list_tag);
}

/* 实现任务调度 */
// schedule的原理之前已介绍，它的功能是将当前线程换下处理器，并在就绪队列中找出下个可运行的程序，将其换上处理器。
/*
调度器主要任务就是读写就绪队列，增删里面的结点，结点是线程PCB中的general_tag，“相当于”线程的PCB，从队列中将其取出时一定要还原成PCB才行。
*/
void schedule() {

   ASSERT(intr_get_status() == INTR_OFF);

   struct task_struct* cur = running_thread(); 
   if (cur->status == TASK_RUNNING) { // 若此线程只是cpu时间片到了,将其加入到就绪队列尾
      ASSERT(!elem_find(&thread_ready_list, &cur->general_tag));
      list_append(&thread_ready_list, &cur->general_tag);
      cur->ticks = cur->priority;     // 重新将当前线程的ticks再重置为其priority;
      cur->status = TASK_READY;
   } else { 
      /* 若此线程需要某事件发生后才能继续上cpu运行,
      不需要将其加入队列,因为当前线程不在就绪队列中。*/
   }

   ASSERT(!list_empty(&thread_ready_list));
   thread_tag = NULL;	  // thread_tag清空
/* 将thread_ready_list队列中的第一个就绪线程弹出,准备将其调度上cpu. */
   thread_tag = list_pop(&thread_ready_list);   
   struct task_struct* next = elem2entry(struct task_struct, general_tag, thread_tag);
   next->status = TASK_RUNNING;
   switch_to(cur, next);
}

/* 初始化线程环境 */
void thread_init(void) {
   put_str("thread_init start\n");
   list_init(&thread_ready_list);
   list_init(&thread_all_list);
/* 将当前main函数创建为线程 */
   make_main_thread();
   put_str("thread_init done\n");
}

