include ../../Makefile.rules
include ../Makefile.rules

SRCS = $(shell find $(CURDIR) -type f -name '*.c')
OBJS = $(addsuffix .o, $(basename $(SRCS)))

test_server.bin: $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $^ $(LISTATIC)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $^

clean:
	rm $(OBJS)

.PHONY: clean