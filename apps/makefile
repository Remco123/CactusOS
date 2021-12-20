CONFIG = ../makefile.config
include ${CONFIG}

LIBINCLUDE := ../lib/include
LIBDIR := ../lib
LIBFILE := ../lib/libcactusos.a
DEBUG := -g

G++PARAMS := -m32 $(DEBUG) -I $(LIBINCLUDE) $(OPTIMIZE_FLAGS) -Wall -fno-omit-frame-pointer -fno-use-cxa-atexit -nostdlib -fno-builtin -fno-exceptions -fno-rtti -fno-leading-underscore -Wno-write-strings -fpermissive -Wno-unknown-pragmas
GCCPARAMS := -m32 $(DEBUG) -I $(LIBINCLUDE) $(OPTIMIZE_FLAGS) -Wall -fno-omit-frame-pointer -nostdlib -fno-builtin -fleading-underscore -Wno-unknown-pragmas
LDPARAMS := -m elf_i386

APPS := $(shell find * -type d)
APPOUT := ../isofiles/apps

SRCFILES := $(shell find $(APPS) -type f \( -name \*.cpp -o -name \*.s -o -name \*.asm -o -name \*.c \)) #Find all the files that end with .cpp/.s/.asm/.c
OBJFILES := $(patsubst %.cpp,%.o,$(patsubst %.s,%.o,$(patsubst %.asm,%.o,$(patsubst %.c,%.o,$(SRCFILES))))) #Replace the .cpp/.s/.asm/.c extension with .o

.PHONY: all clean

all: linker.ld $(LIBFILE) $(OBJFILES)
	for app in $(APPS) ; do \
		echo building $$app.bin to $(APPOUT)/$$app.bin; \
    	i686-elf-ld $(LDPARAMS) -T $< -o $(APPOUT)/$$app.bin $$app/*.o -L$(LIBDIR) --library=cactusos; \
		nm -a $(APPOUT)/$$app.bin | sort -d > $(APPOUT)/$$app.sym; \
	done

clean:
	rm -rf $(OBJFILES) $(LIBFILE)

# Build libcactusos.a with makefile in the LIBDIR
$(LIBFILE):
	cd $(LIBDIR) && $(MAKE)

%.o: %.cpp
	mkdir -p $(@D)
	i686-elf-g++ $(G++PARAMS) -c -o $@ $<

%.o: %.c
	mkdir -p $(@D)
	i686-elf-gcc $(GCCPARAMS) -c -o $@ $<

%.o: %.asm
	mkdir -p $(@D)
	nasm -f elf32 -O0 $< -o $@

%.o: %.s
	mkdir -p $(@D)
	i686-elf-as --32 -o $@ $<