include ../../Makefile.rules
include ../Makefile.rules

SRCS = $(shell find $(CURDIR) -type f \( -name "*.c" -o -name "*.S" \))
OBJS = $(addsuffix .o, $(basename $(SRCS)))

all:
	${MAKE} clean
	${MAKE} libc.so
	${MAKE} libc-static.a
	${MAKE} clean

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
	rm -f $(OBJS)

.PHONY: clean
.PHONY: all
.PHONY: build
.DEFAULT: all