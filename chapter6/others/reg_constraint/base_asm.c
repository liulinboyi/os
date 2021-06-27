#include<stdio.h>
int in_a = 1, in_b = 2, out_sum;
// int a = 500,b = 300;

long a = 1073741824;
int b = 10;
// 1073741824 * 3 = 3221225472
// 1073741824 * 4 = 4294967296
// 1073741824 * 5 = 5368709120
// 1073741824 * 10 = 10737418240
long long result;
long long lowbit;
long long heightbit;
long long result;

/*
             movl 8(%ebp), %eax;\
             movl %eax,result;\
*/
void main() {
     asm("pusha;\
             movl in_a, %eax;\
             movl in_b, %ebx;\
             addl %ebx, %eax;\
             movl %eax, out_sum;\
            popa");
     printf("sum is %d\n",out_sum);
     
     asm("pusha;\
             movl a, %eax;\
             movl b, %ebx;\
             mull %ebx;\
             pushl %eax;\
             pushl %edx;\
             movl %esp,%ebp;\
             movl 4(%ebp),%edx;\
             movl %eax, result;\
             movl %edx, lowbit;\
             movl (%ebp),%edx;\
             movl %edx, heightbit;\
             add $8,%esp;\
            popa");
     printf("result is %lld\n",result);
     printf("lowbit is %lld\n",lowbit);
     printf("heightbit is %lld\n",heightbit);
     printf("\n");
     result = (heightbit << 32) + lowbit;
     printf("%ld * %d = %lld\n", a, b, result);
}


