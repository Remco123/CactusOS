apps:
	i686-elf-gcc -c Test.c -o Test.o -std=gnu99 -ffreestanding -Wall -Wextra
	i686-elf-gcc -T link.ld -o test.bin -ffreestanding -nostdlib Test.o -lgcc -m32
	
