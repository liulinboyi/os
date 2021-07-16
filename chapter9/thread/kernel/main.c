#include "print.h"
#include "init.h"
#include "thread.h"

void k_thread_a(void*);

int main(void) {
   put_str("I am kernel\n");
   init_all();

   thread_start("k_thread_a", 31, k_thread_a, "argA "); // 创建了新线程 线程名字为k_thread_a，优先级为31（此时没什么用，先留着），此线程运行的函数是k_thread_a，它定义在第18行，功能就是打印参数arg。

   while(1);
   return 0;
}

/* 在线程中运行的函数 */
void k_thread_a(void* arg) {     
/* 用void*来通用表示参数,被调用的函数知道自己需要什么类型的参数,自己转换再用 */
   char* para = (char *) arg;
   while(1) {
      put_str(para);
   }
}

