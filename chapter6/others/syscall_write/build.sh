#!/bin/sh

echo "nasm..."
nasm -f elf -o syscall_write.o syscall_write.S

echo "ld..."
ld -m elf_i386 -o syscall_write.bin syscall_write.o

echo "chmod..."
chmod u+x syscall_write.bin

echo "execute...\r\n"
./syscall_write.bin

