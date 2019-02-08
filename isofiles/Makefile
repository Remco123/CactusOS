SDIR	:=	source
ODIR	:=	build
CFILES	:=	$(wildcard $(SDIR)/*.c)
CLIBFILES	:=	$(wildcard $(SDIR)/lib/*.c)
CC		:=	gcc
OBJCOPY	:=	objcopy
LD = ld
OBJS	:=	$(patsubst $(SDIR)/%.c, $(ODIR)/%.o, $(CFILES))
LOBJS	:=	$(patsubst $(SDIR)/lib/%.c, $(ODIR)/%.o, $(CLIBFILES))

CFLAGS = -std=gnu99 -Os -nostdlib -m32 -ffreestanding
LDFLAGS =  -Ttext=0x800000 -m elf_i386 --entry main 

TARGET = $(shell basename $(CURDIR)).bin

$(TARGET): $(ODIR) $(OBJS) $(LOBJS)
	$(LD) $(ODIR)/*.o $(SDIR)/linker/link.ld -o $(ODIR)/program.tmp $(LDFLAGS)
	$(OBJCOPY) -O binary $(ODIR)/program.tmp $(ODIR)/program.bin
	rm -f $(ODIR)/program.tmp

$(ODIR)/%.o: $(SDIR)/%.c
	$(CC) -c -o $@ $< $(CFLAGS)

$(ODIR)/%.o: $(SDIR)/lib/%.c
	$(CC) -c -o $@ $< $(CFLAGS)

$(ODIR):
	@mkdir $@

.PHONY: clean

clean:
	rm -f $(TARGET) $(ODIR)/*.o $(ODIR)/program.bin
