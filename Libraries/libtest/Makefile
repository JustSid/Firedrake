include ../../Makefile.rules
include ../Makefile.rules

SRCS = $(shell find $(CURDIR) -type f \( -name "*.c" -o -name "*.S" \))
OBJS = $(addsuffix .o, $(basename $(SRCS)))

CFLAGS	 = -m32 -Wall -Wextra -pedantic -std=c99 -O2 -fno-stack-protector -fno-builtin -nostdinc -nostdlib -I../.
LDFLAGS = -melf_i386 -static

libtest.a: $(OBJS)
	$(AR) rcs $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $^ 

%.so: %.S
	$(CC) $(ASFLAGS) -c -o $@ $^

clean:
	rm $(OBJS)

.PHONY: clean
