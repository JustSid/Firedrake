include ../../Makefile.rules
include ../Makefile.rules

SRCS = $(shell find $(CURDIR) -type f \( -name "*.c" -o -name "*.S" -o -name "*.cpp" \))
OBJS = $(addsuffix .o, $(basename $(SRCS)))

LDFLAGS += -L../libkernel/

libio.so: $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $^ -lkernel

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $^ 

%.o: %.cpp
	$(CC) $(CPPFLAGS) -c -o $@ $^ 

%.o: %.S
	$(CC) $(ASFLAGS) -c -o $@ $^

clean:
	rm $(OBJS)

.PHONY: clean
