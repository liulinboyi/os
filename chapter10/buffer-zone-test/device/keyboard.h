#ifndef __DEVICE_KEYBOARD_H
#define __DEVICE_KEYBOARD_H
void keyboard_init(void); 
extern struct ioqueue kbd_buf; // 键盘缓冲区是全局数据结构，它在生产者和消费者之间共享，因此在添加生产者线程之前，还要在keyboard.h中添加此缓冲区的声明，这样外部函数就可以访问到它
#endif
