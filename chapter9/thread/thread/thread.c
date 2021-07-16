#include "thread.h"
#include "stdint.h"
#include "string.h"
#include "global.h"
#include "memory.h"

#define PG_SIZE 4096

/* 由kernel_thread去执行function(func_arg) */
static void kernel_thread(thread_func* function, void* func_arg) {
   function(func_arg); 
}

/* 初始化线程栈thread_stack,将待执行的函数和参数放到thread_stack中相应的位置 */
void thread_create(struct task_struct* pthread, thread_func function, void* func_arg) {
   /* 先预留中断使用栈的空间,可见thread.h中定义的结构 */
   pthread->self_kstack -= sizeof(struct intr_stack);

   /* 再留出线程栈空间,可见thread.h中定义 */
   pthread->self_kstack -= sizeof(struct thread_stack);
   // 这里赋值给内核线程栈，此时self_kstack就是指向内核线程栈栈顶，中断栈空间和线程栈空间已经留出来了，thread->self_kstack指向线程栈的最低处
   struct thread_stack* kthread_stack = (struct thread_stack*)pthread->self_kstack;
   kthread_stack->eip = kernel_thread;
   kthread_stack->function = function;
   kthread_stack->func_arg = func_arg;
   kthread_stack->ebp = kthread_stack->ebx = kthread_stack->esi = kthread_stack->edi = 0;
}

/* 初始化线程基本信息 */
void init_thread(struct task_struct* pthread, char* name, int prio) {
   memset(pthread, 0, sizeof(*pthread));
   strcpy(pthread->name, name);
   pthread->status = TASK_RUNNING; 
   pthread->priority = prio;
/* self_kstack是线程自己在内核态下使用的栈顶地址 */
   pthread->self_kstack = (uint32_t*)((uint32_t)pthread + PG_SIZE);
   pthread->stack_magic = 0x19870916;	  // 自定义的魔数
}

/* 创建一优先级为prio的线程,线程名为name,线程所执行的函数是function(func_arg) */
struct task_struct* thread_start(char* name, int prio, thread_func function, void* func_arg) {
/* pcb都位于内核空间,包括用户进程的pcb也是在内核空间 */
   struct task_struct* thread = get_kernel_pages(1);

   init_thread(thread, name, prio); // 初始化刚刚创建的thread线程
   thread_create(thread, function, func_arg); // thread_create接受3个参数，pthread是待创建的线程的指针，function是在线程中运行的函数，func_arg是function的参数。函数的功能是初始化线程栈thread_stack，将待执行的函数和参数放到thread_stack中相应的位置。

   asm volatile ("movl %0, %%esp; pop %%ebp; pop %%ebx; pop %%edi; pop %%esi; ret" : : "g" (thread->self_kstack) : "memory"); // 此汇编代码是开启线程的钥匙
   /*
   输出部分为空，在输入部分，"g" (thread->self_kstack)使thread->self_kstack的值作为输入，采用通用约束g，即内存或寄存器都可以。
   在汇编语句部分，movl %0, %%esp，也就是使thread->self_kstack的值作为栈顶，此时thread->self_kstack指向线程栈的最低处，这是我们在函数thread_create中设定的。
   接下来的这连续4个弹栈操作：pop %%ebp; pop %%ebx; pop %%edi; pop %%esi使之前初始化的0弹入到相应寄存器中。
   */
   return thread;
}
