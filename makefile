# The makefile for the CactusOS project, this file will get more complicated when the OS is in a later stage of development.

##########
# .s files are GAS assembly
# .asm files are nasm assembly
##########
#####################
#xHCI
# nec-usb-xhci
# qemu-xhci
#EHCI
# usb-ehci
# ich9-usb-ehci1
#UHCI
# ich9-usb-uhci1
# ich9-usb-uhci2
# ich9-usb-uhci3
# piix3-usb-uhci
# piix4-usb-uhci
# vt82c686b-usb-uhci
#OHCI
# sysbus-ohci
# pci-ohci
#######################

INCLUDEDIRS := kernel/include
QEMUOPTIONS := -boot d -device ich9-usb-ehci1 -device VGA,edid=on -trace events=../qemuTrace.txt -drive if=none,id=usbstick,file=../usbdisk.img,format=raw -device usb-storage,id=stick,drive=usbstick

G++PARAMS := -m32 -g -I $(INCLUDEDIRS) -fno-use-cxa-atexit -nostdlib -fno-builtin -fno-exceptions -fno-rtti -fno-leading-underscore -Wno-write-strings -fpermissive -Wall
GCCPARAMS := -m32 -g -I $(INCLUDEDIRS) -nostdlib -fno-builtin -Wall -fleading-underscore
ASPARAMS := --32
LDPARAMS := -m elf_i386

KRNLSRCDIR := kernel/src
KRNLOBJDIR := kernel/obj

KRNLFILES := $(shell find $(KRNLSRCDIR) -type f \( -name \*.cpp -o -name \*.s -o -name \*.asm -o -name \*.c \)) #Find all the files that end with .cpp/.s/.asm/.c
KRNLOBJS := $(patsubst %.cpp,%.o,$(patsubst %.s,%.o,$(patsubst %.asm,%.o,$(patsubst %.c,%.o,$(KRNLFILES))))) #Replace the .cpp/.s/.asm/.c extension with .o
KRNLOBJS := $(subst $(KRNLSRCDIR),$(KRNLOBJDIR),$(KRNLOBJS)) #Replace the kernel/src part with kernel/obj


####################################
#C++ source files
####################################
$(KRNLOBJDIR)/%.o: $(KRNLSRCDIR)/%.cpp
	mkdir -p $(@D)
	i686-elf-g++ $(G++PARAMS) -c -o $@ $<

####################################
#C source files
####################################
$(KRNLOBJDIR)/%.o: $(KRNLSRCDIR)/%.c
	mkdir -p $(@D)
	i686-elf-gcc $(GCCPARAMS) -c -o $@ $<

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
	mkdir -p $(@D)
	nasm -f elf32 -O0 $< -o $@



CactusOS.bin: kernel/linker.ld $(KRNLOBJS)
	i686-elf-ld $(LDPARAMS) -T $< -o $@ $(KRNLOBJS)
	cd lib/ && $(MAKE)
	cd apps/ && $(MAKE)

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

.PHONY: clean qemu kdbg run filelist serialDBG qemuDBG fastApps
clean:
	rm -rf $(KRNLOBJDIR) CactusOS.bin CactusOS.iso
	cd lib/ && $(MAKE) clean
	cd apps/ && $(MAKE) clean
	rm -rf isofiles/apps/*.bin

qemu: CactusOS.iso
	qemu-system-i386 -cdrom CactusOS.iso -serial stdio $(QEMUOPTIONS)

qemuDBG: CactusOS.iso
	qemu-system-i386 -cdrom CactusOS.iso -serial stdio $(QEMUOPTIONS) -s -S &

qemuGDB: CactusOS.iso
	qemu-system-i386 -cdrom CactusOS.iso $(QEMUOPTIONS) -serial pty &
	gdb -ex 'file CactusOS.bin' -ex 'target remote /dev/pts/1' -q

run: CactusOS.iso
	(killall VirtualBox && sleep 1) || true
	virtualbox --startvm 'CactusOS' &
	rm "../Virtualbox Serial Log.txt"
	echo "" > "../Virtualbox Serial Log.txt"
	tail -f "../Virtualbox Serial Log.txt"

serialDBG:
	gcc -o tools/serialDebugger/a.out tools/serialDebugger/main.c
	sudo ./tools/serialDebugger/a.out

kdbg: CactusOS.iso
	qemu-system-i386 -cdrom CactusOS.iso $(USBOPTIONS) -serial stdio -s -S &
	kdbg -r localhost:1234 CactusOS.bin

# Only rebuild LIBCactusOS and the apps without recompiling the kernel
fastApps:
	rm -rf isofiles/apps/*.bin
	cd lib/ && $(MAKE) clean && $(MAKE)
	cd apps/ && $(MAKE) clean && $(MAKE)
	rm CactusOS.iso

filelist:
	@echo "Source Files:"
	@echo -$(KRNLFILES)
	@echo "Object Files:"
	@echo -$(KRNLOBJS)