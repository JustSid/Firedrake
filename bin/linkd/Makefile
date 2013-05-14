include ../../Makefile.rules
include ../Makefile.rules

SRCS = $(shell find $(CURDIR) -type f -name '*.c')
OBJS = $(addsuffix .o, $(basename $(SRCS)))

LDFLAGS += -TMakefile_ld

linkd.bin: $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $^ -lcrt -lc-static

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $^

clean:
	rm $(OBJS)

.PHONY: clean