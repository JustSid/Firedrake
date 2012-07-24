include ../../Makefile.rules
include ../Makefile.rules

SRCS = $(shell find $(CURDIR) -type f -name '*.c')
OBJS = $(addsuffix .o, $(basename $(SRCS)))

LDFLAGS += -L../../Libraries/libcrt/
LDFLAGS += -L../../Libraries/libtest/

hellostatic.bin: $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $^ -lcrt -ltest

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $^

clean:
	rm $(OBJS)

.PHONY: clean