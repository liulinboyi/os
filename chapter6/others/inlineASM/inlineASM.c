#include <stdio.h>

/* 大家注意到没有，inlineASM.c中的变量count和str定义为全局变量。对的，在基本内联汇编中，若要引用C变量，只能将它定义为全局变量。如果定义为局部变量，链接时会找不到这两个符号 */
char* str="hello,world this is my first inlineASM.\n";
int len = 0;
int count = 0;
int result = 0;

int cont_str(char *s)
{
    int i = 0;
    while ( str[i] != 0) {
    	printf("%d\n",str[i]);
    	i++;
    }
    return i;
}

void main(){

len = cont_str(str);
printf("%d\n",len);
printf("\n");
/*寄存器前面加前缀%，立即数前面加前缀$，操作数由左到右的顺序。*/
/* write的功能是把buf指向的缓冲区中的count个字节写入fd指向的文件描述符，执行成功后返回写入的字节数，失败则返回-1。 write(1,"hello,world\n",4); */
/*
eax寄存器用来存储子功能号（寄存器eip、ebp、esp是不能使用的）。5个参数存放在以下寄存器中，传送参数的顺序如下。
（1）ebx存储第1个参数。
（2）ecx存储第2个参数。
（3）edx存储第3个参数。
（4）esi存储第4个参数。
（5）edi存储第5个参数。
*/
asm volatile (
    "pusha;/* 将8个通用寄存器压栈 */\
     movl $4,%eax;/* 传入第4号系统调用，这就是write的调用号*/\
     movl $1,%ebx;/* fd */\
     movl str,%ecx;/* buffer */\
     movl len,%edx;/* buffer_len */\
     int $0x80;/* 执行系统调用0x80 */\
     mov %eax,count;/* 获取write的返回值，返回值都是存储在eax寄存器中，所以将其复制到变量count中。 */\
     mov %eax, result;\
     popa;/* 将8个通用寄存器出栈 */\
     "
);
printf("The system caller 0x80's return value is %d\n",result);

}

