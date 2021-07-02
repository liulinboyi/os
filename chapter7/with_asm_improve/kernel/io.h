// 和中断相关的数据准备好了，接下来只要把中断代理8259A设置好就可以啦
/**************	 机器模式   ***************
	 b -- 输出寄存器QImode名称,即寄存器中的最低8位:[a-d]l。
	 w -- 输出寄存器HImode名称,即寄存器中2个字节的部分,如[a-d]x。

	 HImode
	     “Half-Integer”模式，表示一个两字节的整数。 
	 QImode
	     “Quarter-Integer”模式，表示一个一字节的整数。 
*******************************************/

/*
解惑:

与其他c文件不同的是我们把函数的实现部分直接放到了以.h为结尾的头文件中，这似乎显得有些怪异。
我们平时都是把函数体放在.c文件中，头文件只用于存放函数声明，如果函数是全局作用域的话，链接后，外部文件就可以调用该函数了，所以，一般情况下，头文件都作为功能模块的接口而存在，头文件中对应的函数在程序中也只会存在一份。而我们的io.h却是函数的实现，并且，里面各函数的作用域都是static，这表明该函数仅在本文件中有效，对外不可见。这意味着，凡是包含io.h的文件，都会获得一份io.h中所有函数的拷贝，也就是说同样功能的函数在程序中会存在多个副本，这样程序体积就会大一些。看上去哥们还是“清醒”的，但为什么还是干这样的“糊涂”事呢？
是这样的，这里的函数并不是普通的函数，它们都是对底层硬件端口直接操作的，通常由设备的驱动程序来调用，不用说，为了快速响应，函数调用上需要更加高效。而且，操作系统是为了让用户程序编写、执行更加方便才诞生的，归根结底是为了用户程序服务，所以它会让处理器的大多数时间花在3特权级的用户程序上。为了让处理器更多地为用户程序服务，操作系统（包括硬件驱动程序）必须减少自己占用处理器的时间，所以，对硬件端口的操作，只要求一个字—快。
但一般的函数调用需要涉及到现场保护及恢复现场，即函数调用前要把相关的栈、返回地址（CS和EIP）保存到栈中，函数执行完返回后再将它们从栈中恢复到寄存器。栈毕竟是内存，速度低很多，而且入栈、出栈这么多内存操作，对于想方设法提速的内核程序来说是无法忍受的。
因此，为了提速，在我们的实现中，函数的存储类型都是static，并且加了inline关键字，它建议处理器将函数编译为内嵌的方式。内嵌函数大家都清楚吧，就是将所调用的函数体的内容，在该函数的调用处，原封不动地展开，这样编译后的代码中将不包含call指令，也就不属于函数调用了，而是顺次执行。虽然这会让程序大一些，但减少了函数调用及返回时的现场保护及恢复工作，提升了效率还是值得的。


*/

/*
内联汇编的格式是：asm [volatile] (“assembly code” : output : input : clobber/modify):
*/

/*
实际上操作码就是指定操作数为寄存器中的哪个部分。咱们这里不用关注太多，初步了解h、b、w、k这几个操作码就够了，以后有需要时再说。
寄存器按是否可单独使用，可分成几个部分，拿eax举例。
-　低部分的一字节：al
-　高部分的一字节：ah
-　两字节部分：ax
-　四字节部分：eax
h –输出寄存器高位部分中的那一字节对应的寄存器名称，如ah、bh、ch、dh。
b –输出寄存器中低部分1字节对应的名称，如al、bl、cl、dl。
w –输出寄存器中大小为2个字节对应的部分，如ax、bx、cx、dx。
k –输出寄存器的四字节部分，如eax、ebx、ecx、edx。
*/

#ifndef _LIB_IO_H
# define _LIB_IO_H

# include "stdint.h"

/**
 *  向指定的端口写入一个字节的数据.
 */ 
/* 向端口port写入一个字节*/
/*
outb函数接受两个参数，参数port是16位无符号整型的端口号，此类型可容纳Intel所支持的65536个端口号，参数data是1字节的整型数据，outb的功能是将data中的1字节数据写入port所指向的端口。
*/
static inline void outb(uint16_t port, uint8_t data) {
/*********************************************************
 a表示用寄存器al或ax或eax,对端口指定N表示0~255, d表示用dx存储端口号, 
 %b0表示对应al,%w1表示对应dx */ 
// outb指令格式为outb %al，%dx，其中%al是源操作数，指的是8位数据，%dx是目的操作数，指的是数据所写入的端口。
    asm volatile ("outb %b0, %w1" : : "a" (data), "Nd" (port));   
}

/**
 * 将addr起始处的word_cnt个字节写入端口port.
 */ 
/* 将addr处起始的word_cnt个字写入端口port 注意，是以2字节为单位的。 */
static inline void outsw(uint16_t port, const void* addr, uint32_t word_cnt) {
/*********************************************************
   +表示此限制即做输入又做输出.
   outsw是把ds:esi处的16位的内容写入port端口, 我们在设置段描述符时, 
   已经将ds,es,ss段的选择子都设置为相同的值了,此时不用担心数据错乱。*/
    asm volatile ("cld; rep outsw" : "+S" (addr), "+c" (word_cnt) : "d" (port));
}

/**
 * 将从端口port读入的一个字节返回.
 */ 
static inline uint8_t inb(uint16_t port) {
    uint8_t data;
    asm volatile ("inb %w1, %b0" : "=a" (data) : "Nd" (port));
    return data;
}

/**
 * 将从port读取的word_cnt字节写入addr.
 */
/* 将从端口port读入的word_cnt个字写入addr */
static inline void insw(uint16_t port, void* addr, uint32_t word_cnt) {
/******************************************************
   insw是将从端口port处读入的16位内容写入es:edi指向的内存,
   我们在设置段描述符时, 已经将ds,es,ss段的选择子都设置为相同的值了,
   此时不用担心数据错乱。*/
 // "+D" (addr)表示用寄存器约束D将变量addr的值约束到EDI中
 // INSW: 从 DX 指定的 I/O 端口将字输入 ES:(E)DI 指定的内存位置
    asm volatile ("cld; rep insw" : "+D" (addr), "+c" (word_cnt) : "d" (port) : "memory");
}

#endif