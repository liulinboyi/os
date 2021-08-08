#ifndef __LIB_USER_SYSCALL_H
#define __LIB_USER_SYSCALL_H
#include "stdint.h"
// 以后再增加新的系统调用后还需要把新的子功能号添加到此结构中
enum SYSCALL_NR {
   SYS_GETPID
};
uint32_t getpid(void);
#endif

