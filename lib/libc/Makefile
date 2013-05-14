include ../../Makefile.rules
include ../Makefile.rules

SRCS = $(shell find $(CURDIR) -type f \( -name "*.c" -o -name "*.S" \))
OBJS = $(addsuffix .o, $(basename $(SRCS)))

all: build libc.so libc-static.a

build: $(OBJS)
%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $^ 

%.o: %.S
	$(CC) $(ASFLAGS) -c -o $@ $^


libc.so: $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $^

libc-static.a: $(OBJS)
	$(AR) rcs $@ $^

clean:
	rm $(OBJS)

.PHONY: clean
.PHONY: all
.PHONY: build
.DEFAULT: all