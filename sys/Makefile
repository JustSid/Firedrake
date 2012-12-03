include ../Makefile.rules

SRCS = $(shell find $(CURDIR) -type f \( -name "*.c" -o -name "*.S" \))
OBJS = $(addsuffix .o, $(basename $(SRCS)))


ASFLAGS  = -m32 -I.
CFLAGS 	 = -m32 -std=c11 -Wall -Wextra -mno-sse -mno-mmx -nostdlib -nostdinc -fno-stack-protector -fno-omit-frame-pointer -fno-builtin -O2 -pedantic-errors -I.
LDFLAGS  = -T Makefile.ld

firedrake: $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $^

%.o: %.S
	$(CC) $(ASFLAGS) -c -o $@ $^

clean:
	rm $(OBJS)

.PHONY: clean

