## C和汇编混合编程的方法啦，确切说是方法之一

> 首先要记住：
> extern来声明所需要的外部函数名，在本文件中没有 												- c与汇编一致
> global关键字，global将符号导出为全局属性，对程序中的所有文件可见，这样其他外部文件中也可以引用被global导出的符号啦，无论该符号是函数，还是变量 - 只能在汇编中使用

### C_with_S_c.c

```
extern void asm_print(char*,int); // 通知编译器这个函数并不在当前文件中定义
/*
我们知道它定义在文件C_with_S_S.S中，但编译器是不知道的，所以只能在链接阶段将此函数重新定位，编排地址。
*/
// c_print的实现，在它的内部，它又调用了汇编代码C_with_S_S.S中的函数asm_print，经过第1行的声明，我们要给它提供两个参数：字符串的起始地址及长度。
/*
特别强调一下，由于这里并不打算把C标准库也链接进来，所以在求字符串长度时，我们不能用string.h中的strlen函数。也就是说即使include <string.h>将其strlen的声明加进来，没有strlen实现本身所在的.o文件也是不行的。函数声明的作用是：一方面告诉编译器该函数的参数所需要的栈空间大小及返回值，这样编译器能为其准备好执行环境；另一方面如果该函数是在外部文件中定义的，一定要在链接阶段时将其对应的目标文件一块链接进来。
*/
void c_print(char* str) {
     int len=0;
     while(str[len++]); // 通过while循环求字符串的长度。字符串结尾必须是空字符'\0'才行，否则while就是死循环了。这个字符串是代码C_with_S_S.S提供的，我们转过去看看
     asm_print(str, len);
}

```

### C_with_S_S.S
```
section .data
str: db "asm_print says hello world!", 0xa, 0
;0xa是换行符,0是手工加上的字符串结束符\0的ASCII码 定义待打印的字符串时，在结尾人为地加了个0，它就是空字符'\0'的ASCII码。
str_len equ $-str

section .text
extern c_print ; 通知编译器这个函数并不在当前文件中定义
global _start ; 将_start导出为全局符号，为的是给链接器用的 在汇编语言中导出符号名用global关键字，这在之前说_start时大伙已有所耳闻，global将符号导出为全局属性，对程序中的所有文件可见，这样其他外部文件中也可以引用被global导出的符号啦，无论该符号是函数，还是变量。
_start:
;;;;;;;;;;;; 调用c代码中的函数c_print ;;;;;;;;;;;
     push str ; 传入参数
     call c_print ; 调用c函数
     add esp,4 ; 回收栈空间

 ;;;;;;;;;;;;;;;;;;; 退出程序 ;;;;;;;;;;;;;;;;;;;;
     mov eax,1 ; 第1号子功能是exit系统调用
     int 0x80 ; 发起中断，通知Linux完成请求的功能

 global asm_print ; 相当于asm_print（str,size）
 asm_print:
     push ebp ; 备份ebp
     mov ebp,esp
     mov eax,4 ; 第4号子功能是write系统调用
     mov ebx, 1 ; 此项固定为文件描述符1，标准输出（stdout）指向屏幕
     mov ecx, [ebp+8] ; 第1个参数
     mov edx, [ebp+12] ; 第2个参数
     int 0x80 ; 发起中断，通知Linux完成请求的功能
     pop ebp ; 恢复ebp
     ret
```



