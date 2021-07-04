#include "print.h"
#include "init.h"
#include "debug.h"
int main(void) {
   put_str("I am kernel\n");
   init_all();
   ASSERT(1==2);
   // 这里把开中断sti的内联汇编去掉了，因为之前是为了演示中断机制才打开中断的，目前暂时将其关闭，等真正时机成熟咱们再打开不迟。
   while(1);
   return 0;
}
