LIBINCLUDE := ../lib/include
LIBDIR := ../lib
LIBFILE := ../lib/libcactusos.a

GCCPARAMS := -m32 -g -I $(LIBINCLUDE) -fno-use-cxa-atexit -nostdlib -fno-builtin -fno-exceptions -fno-rtti -fno-leading-underscore -Wno-write-strings -fpermissive -Wall
LDPARAMS := -m elf_i386

APPS := init
APPOUT := ../isofiles/apps

SRCFILES := $(shell find $(APPS) -type f \( -name \*.cpp -o -name \*.s -o -name \*.asm \))
OBJFILES := $(patsubst %.cpp,%.o,$(patsubst %.s,%.o,$(patsubst %.asm,%.o,$(SRCFILES)))) #Replace the .cpp/.s/.asm extension with .o



all: linker.ld $(LIBFILE) $(OBJFILES)
	for app in $(APPS) ; do \
		echo building $$app.bin to $(APPOUT)/$$app.bin; \
    	i686-elf-ld $(LDPARAMS) -T $< -o $(APPOUT)/$$app.bin $$app/*.o -L$(LIBDIR) --library=cactusos; \
	done



# Build libcactusos.a with makefile in the LIBDIR
$(LIBFILE):
	cd $(LIBDIR) && $(MAKE)

%.o: %.cpp
	mkdir -p $(@D)
	i686-elf-g++ $(GCCPARAMS) -c -o $@ $<