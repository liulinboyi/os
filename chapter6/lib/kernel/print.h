/*
为防止头文件被重复包含，避免头文件中的变量等出现重复定义的情况。可以用条件编译指令#ifndef和#endif来封闭文件的内容，把要定义的内容放在它们之中。
*/
/**
 * 为print.asm提供方便引用的头文件定义.
   前2行是以print.h所在的路径定义了这个宏
   LIB_KERNEL_PRINT_H，以该宏来判断是否重复包含。
 */
#ifndef _LIB_KERNEL_PRINT_H
#define _LIB_KERNEL_PRINT_H

# include "stdint.h" // include指令包含了“stdint.h”。这里是用双引号括住了stdint.h，目的是包含自己指定的文件，如果是用尖括号<>括住的，这是让编译器到系统头文件所在的目录中找所包含的文件，这个目录通常是/usr/include。

void put_char(uint8_t char_asci); // 无符号8位整型变量（其实就是unsigned char）

/**
 *  字符串打印，必须以\0结尾.
 */ 
void put_str(char* message);

/**
 * 以16进制的形式打印数字.
 */ 
void put_int(uint32_t num);

#endif // #endif是与条件编译#ifndef相配合的结束指令，固定用法。
