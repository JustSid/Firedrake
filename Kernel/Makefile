CSRCS = $(shell find $(CURDIR) -type f -name '*.c')
SSRCS = $(shell find $(CURDIR) -type f -name '*.S')

OBJS = $(addsuffix .o, $(basename $(CSRCS)))
OBJS += $(addsuffix .so, $(basename $(SSRCS)))

ASFLAGS  = -m32 -c -o
CFLAGS 	= -m32 -Wall -fno-stack-protector -std=c99 -nostdlib -fno-builtin -fno-exceptions -fno-leading-underscore -pedantic-errors -I. -c -o
LDFLAGS  = -TMakefile.ld -o

firedrake: $(OBJS)
	$(LD) $(LDFLAGS) $@ $^

%.o: %.c
	$(CC) $(CFLAGS) $@ $^

%.so: %.S
	$(CC) $(ASFLAGS) $@ $^

clean:
	rm $(OBJS)

.PHONY: clean

