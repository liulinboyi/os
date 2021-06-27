#include <stdio.h>
void main() {
/*
修饰符'+'也只用在output中，但它具备读、写的属性，也就是它既可作为输入，同时也可以作为输出，所以省去了在input中声明约束。
*/
     int in_a = 1, in_b = 2;
     asm("addl %%ebx, %%eax;":"+a"(in_a):"b"(in_b));
     printf("in_a is %d\n", in_a);
}


