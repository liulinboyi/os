build/test2.o:src/test2.c
	@gcc -c -o build/test2.o src/test2.c
build/test1.o:src/test1.c
	@gcc -c -o build/test1.o src/test1.c

file = build/test1.o build/test2.o

dist/test.bin:$(file)
	@gcc -o dist/test.bin $(file)
all:dist/test.bin
	@echo "compile done"

PHONY:clean
clean:
	@rm -rf ./build/*.o ./dist/*.bin
	@echo "clean done"

