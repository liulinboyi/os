#include<stdio.h>
void main() {
    int in_a = 0x1234, in_b = 0;
    // in_a的低16位复制到in_b中。
    asm("movw %1, %0":"=m"(in_b):"a"(in_a));
    printf("in_b now is 0x%x\n", in_b);
}
 


