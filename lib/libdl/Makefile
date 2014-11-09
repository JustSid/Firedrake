include ../../Makefile.rules
include ../Makefile.rules

SRCS = $(shell find $(CURDIR) -type f \( -name "*.c" -o -name "*.S" \))
OBJS = $(addsuffix .o, $(basename $(SRCS)))

libdl.so: $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $^ 

%.o: %.S
	$(CC) $(ASFLAGS) -c -o $@ $^

clean:
	rm $(OBJS)

.PHONY: clean