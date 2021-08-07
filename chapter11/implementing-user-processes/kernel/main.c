#include "print.h"
#include "init.h"
#include "thread.h"
#include "interrupt.h"
#include "console.h"
#include "process.h"

void k_thread_a(void*);
void k_thread_b(void*);
void u_prog_a(void);
void u_prog_b(void);
int test_var_a = 0, test_var_b = 0;

int main(void) {
   put_str("I am kernel\n");
   init_all();

   // 创建了两个线程和两个进程，至此我们的系统中并行了4个任务
   thread_start("k_thread_a", 31, k_thread_a, "argA ");
   thread_start("k_thread_b", 31, k_thread_b, "argB ");
   process_execute(u_prog_a, "user_prog_a"); // 用户进程的创建是由函数process_execute完成的
   process_execute(u_prog_b, "user_prog_b"); // 用户进程的创建是由函数process_execute完成的

   intr_enable();
   while(1);
   return 0;
}

/* 在线程中运行的函数 */
void k_thread_a(void* arg) {     
   char* para = arg;
   while(1) {
      // 变量在用户进程中自增，由高特权级的线程来帮忙打印
      console_put_str(" v_a:0x");
      console_put_int(test_var_a);
   }
}

/* 在线程中运行的函数 */
void k_thread_b(void* arg) {     
   char* para = arg;
   while(1) {
      // 变量在用户进程中自增，由高特权级的线程来帮忙打印
      console_put_str(" v_b:0x");
      console_put_int(test_var_b);
   }
}

/* 测试用户进程 */
void u_prog_a(void) {
   while(1) {
      test_var_a++;
   }
}

/* 测试用户进程 */
void u_prog_b(void) {
   while(1) {
      test_var_b++;
   }
}
