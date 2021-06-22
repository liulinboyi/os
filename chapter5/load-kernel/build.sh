#!/bin/sh

# shopt -s expand_aliases

echo "Creating disk.img..."
bximage -mode=create -hd=10M -q disk.img

echo "Compiling..."
gcc -m32 -c -o kernel/main.o kernel/main.c
ld -m elf_i386 kernel/main.o -Ttext 0xc0001500 -e main -o kernel/kernel.bin
nasm -I include/ -o mbr.bin mbr.asm
nasm -I include/ -o loader.bin loader.asm

echo "Writing mbr, loader and kernel to disk..."
dd if=mbr.bin of=disk.img bs=512 count=1 conv=notrunc
dd if=loader.bin of=disk.img bs=512 count=4 seek=2 conv=notrunc
dd if=kernel/kernel.bin of=disk.img bs=512 count=200 seek=9 conv=notrunc

echo "Now start bochs and have fun!"
bochs -f bochsrc 
