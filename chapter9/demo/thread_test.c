#include<stdio.h> 
#include<pthread.h> // POSIX版本线程库

// 创建线程
void* thread_func(void* _arg) { 
  unsigned int * arg = _arg; 
  printf(" new thread: my tid is %u\n ",*arg);
}

void main() { 
 pthread_t new_thread_id; 
 /*
 int pthread_create (pthread_t *　restrict 　newthread, // newthread用于存储新创建线程的id，也就是tid，这里保存在pthread_t类型的变量new_thread_id中。
 __const pthread_attr_t *__restrict __attr, // attr用于指定线程的类型，我们这里就用默认类型就好，因此实参是NULL。
 void *(*__start_routine) (void *), // start_routine是个函数指针，确切地说是个返回值为void*、参数为void*的函数指针，用来指定线程中所调用的函数的地址，或者说是在线程中运行的函数的地址。这里的实参就是在上面定义的函数thread_func，也就是说让新创建的线程去调用执行thread_func函数。
 void *__restrict __arg) __THROW __nonnull ((1, 3)); // arg，它是用来配合第3 个参数的，是给在线程中运行的函数start_routine的参数，我们此处把new_thread_id传给thread_func。注意，由于给start_routine函数做参数的只有这一个形参，当参数多于一个时，最好把参数封装为一个结构体，把此结构体地址传给arg，然后在start_routine指向的函数体中再去解析参数。
*/
 pthread_create(&new_thread_id,NULL,thread_func,&new_thread_id); // pthread_create函数的返回值若为0，则表示创建线程成功，否则就表示出错码。
 printf("main thread: my tid is %u\n ", (unsigned int) pthread_self());
 usleep(100);
}
 
 
