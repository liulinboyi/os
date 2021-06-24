#!/bin/sh

nasm -f elf -o C_with_S_S.o C_with_S_S.S
gcc -m32 -c -o C_with_S_c.o C_with_S_c.c 
ld -m elf_i386 -o C_with.bin C_with_S_c.o C_with_S_S.o
./C_with.bin

