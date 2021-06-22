## 常用命令
> 重要：推荐使用gcc 4.4.x版本

## [ubuntu20_gcc_降级](./ubuntu20_gcc_降级_wubing9356的博客_CSDN博客)

```
gcc -m32 -c -o kernel/main.o kernel/main.c

file kernel/main.o

nm kernel/main.o

ld -m elf_i386 kernel/main.o -Ttext 0xc0001500 -e main -o kernel/kernel.bin

sh ../../tool/xxd.sh kernel/kernel.bin 0 300

readelf -e kernel/kernel.bin
```

在Linux下用于链接的程序是ld，链接有一个好处，可以指定最终生成的可执行文件的起始虚拟地址。它是用-Ttext参数来指定的，所以咱们可以执行以下命令完成链接。
ld kernel/main.o -Ttext 0xc0001500 -e main -o kernel/kernel.bin
从左到右说一下参数，-Ttext 指定起始虚拟地址为0xc0001500，这个地址是设计好的，为什么用这个地址，咱们将来在加载内核时会告诉大家，在此大伙儿先淡定一下。其中–o的意义也是指定输出的文件名，至于-e，还是要看一下官方帮助。
-e和--entry一样，字面上的意思是用来指定程序的起始地址。注意，不要被迷惑了，虽然说是指定起始地址，但参数不仅可以是数字形式的地址，而且可以是符号名，这和汇编中的标号也是地址是一样的道理。总之它用来指定程序从哪里开始执行。

由于咱们用的是C语言写的程序，想到的编译器自然是大名鼎鼎的gcc，所以我们用gcc编译该程序的参数是：
gcc -c -o kernel/main.o kernel/main.c

编译成目标文件时，我们可以用file命令检查一下main.o的状态，如file kernel/main.o

目标文件是可重定位文件，其中的符号都尚未“定位”，也就是符号（变量名，函数名）的地址尚未确定，这一点我们可以用Linux的nm命令来查看



p_vaddr属性 00 80 04 08 => 08048000

程序的入口虚拟地址p_entry是0xc0101500，第一个段的起始虚拟地址p_vaddr为0x08048000，并且第一个段在文件内的偏移量为0

readelf命令从名字上就能看出它的使命：读出elf文件的信息。



0x000000000d9c error


0x000000000da0



