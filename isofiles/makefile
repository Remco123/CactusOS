apps:
	i686-elf-gcc -c init.c -o init.o -std=gnu99 -ffreestanding -Wall -Wextra
	i686-elf-gcc -T link.ld -o init.bin -ffreestanding -nostdlib init.o -lgcc -m32
	rm init.o
	
