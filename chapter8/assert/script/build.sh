#!/bin/sh

mkdir build

bximage -mode=create -hd=10M -q disk.img
nasm -I boot/include/ -o build/mbr.bin boot/mbr.S
nasm -I boot/include/ -o build/loader.bin boot/loader.S
gcc -m32 -I lib/kernel/ -c -o build/debug.o kernel/debug.c
gcc -m32 -I lib/kernel/ -c -o build/timer.o device/timer.c
gcc -m32 -I lib/kernel/ -I lib/ -I kernel/ -c -fno-builtin -o build/main.o kernel/main.c
gcc -m32 -I lib/kernel/ -I lib/ -I kernel/ -I device/ -c -fno-builtin -o build/init.o kernel/init.c
gcc -m32 -I lib/kernel/ -I lib/ -I kernel/ -c -fno-builtin -o build/interrupt.o kernel/interrupt.c
# 编译汇编程序,生成目标文件
nasm -f elf -o build/kernel.o kernel/kernel.S
nasm -f elf -o build/print.o lib/kernel/print.S
# 链接所有目标文件,在build目录下生成kernel.bin
ld -m elf_i386 -Ttext 0xc0001500 -e main -o build/kernel.bin build/main.o build/init.o build/interrupt.o build/print.o build/kernel.o build/timer.o build/debug.o
# 将kernel.bin用dd命令写入虚拟机磁盘
dd if=build/mbr.bin of=disk.img bs=512 count=1 conv=notrunc
dd if=build/loader.bin of=disk.img bs=512 count=4 seek=2 conv=notrunc
dd if=build/kernel.bin of=disk.img bs=512 count=200 seek=9 conv=notrunc
bochs -f bochsrc 

