# The makefile for the CactusOS project, this file will get more complicated when the OS is in a later stage of development.

##########
# .s files are GAS assembly
# .asm files are nasm assembly
##########

INCLUDEDIRS := kernel/include

GCCPARAMS := -m32 -g -I $(INCLUDEDIRS) -fno-use-cxa-atexit -nostdlib -fno-builtin -fno-exceptions -fno-rtti -fno-leading-underscore -Wno-write-strings -fpermissive -Wall
ASPARAMS := --32
LDPARAMS := -m elf_i386

KRNLSRCDIR := kernel/src
KRNLOBJDIR := kernel/obj

KRNLFILES := $(shell find $(KRNLSRCDIR) -type f \( -name \*.cpp -o -name \*.s -o -name \*.asm \)) #Find all the files that end with .cpp/.s/.asm
KRNLOBJS := $(patsubst %.cpp,%.o,$(patsubst %.s,%.o,$(patsubst %.asm,%.o,$(KRNLFILES)))) #Replace the .cpp/.s/.asm extension with .o
KRNLOBJS := $(subst $(KRNLSRCDIR),$(KRNLOBJDIR),$(KRNLOBJS)) #Replace the kernel/src part with kernel/obj


####################################
#C++ source files
####################################
$(KRNLOBJDIR)/%.o: $(KRNLSRCDIR)/%.cpp
	mkdir -p $(@D)
	i686-elf-g++ $(GCCPARAMS) -c -o $@ $<

####################################
#GAS assembly files
####################################
$(KRNLOBJDIR)/%.o: $(KRNLSRCDIR)/%.s
	mkdir -p $(@D)
	i686-elf-as $(ASPARAMS) -o $@ $<

####################################
#NASM assembly files
####################################
$(KRNLOBJDIR)/%.o: $(KRNLSRCDIR)/%.asm
	nasm -f elf $< -o $@



CactusOS.bin: kernel/linker.ld $(KRNLOBJS)
	i686-elf-ld $(LDPARAMS) -T $< -o $@ $(KRNLOBJS)

CactusOS.iso: CactusOS.bin
	cp -r isofiles/. iso
	mkdir iso/boot
	mkdir iso/boot/grub
	cp CactusOS.bin iso/boot/CactusOS.bin
	cp grub.cfg iso/boot/grub/grub.cfg
	grub-mkrescue --output=CactusOS.iso iso
	rm -rf iso

install: CactusOS.iso
	cp $< /media/sf_Mint_OSDev/CactusOS.iso
	cp CactusOS.bin /media/sf_Mint_OSDev/CactusOS.bin

.PHONY: clean qemu kdbg run filelist
clean:
	rm -rf $(KRNLOBJDIR) CactusOS.bin CactusOS.iso

qemu: CactusOS.iso
	qemu-system-i386 -cdrom CactusOS.iso -serial stdio

run: CactusOS.iso
	(killall VirtualBox && sleep 1) || true
	virtualbox --startvm 'CactusOS' 

kdbg: CactusOS.iso
	qemu-system-i386 -cdrom CactusOS.iso -serial stdio -s -S &
	kdbg -r localhost:1234 CactusOS.bin

filelist:
	@echo "Source Files:"
	@echo -$(KRNLFILES)
	@echo "Object Files:"
	@echo -$(KRNLOBJS)