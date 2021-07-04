build/test2.o:src/test2.c
	@gcc -c -o build/test2.o src/test2.c
build/test1.o:src/test1.c
	@gcc -c -o build/test1.o src/test1.c
dist/test.bin:build/test1.o build/test2.o
	@gcc -o dist/test.bin build/test1.o build/test2.o
all:dist/test.bin
	@echo "compile done"

PHONY:clean
clean:
	@rm -rf ./build/*.o ./dist/*.bin
	@echo "clean done"

