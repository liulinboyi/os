#include "print.h"
#include "init.h"
#include "string.h"
int main(void) {
   put_str("I am kernel\n");
   init_all();
   char* s = "hello world";
   int l = strlen(s);
   put_str(s);
   put_str(" length are 0x");
   put_int(l);
   while(1);
   return 0;
}
