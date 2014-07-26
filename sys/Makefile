include ../Makefile.rules

SRCS = $(shell find $(CURDIR) -type f \( -name "*.cpp" -o -name "*.c" -o -name "*.S" \))
OBJS = $(addsuffix .o, $(basename $(SRCS)))

ASFLAGS  = -m32 -I. -no-integrated-as
CFLAGS 	 = -m32 -std=c99 -O2 -D __KERNEL -Wall -Wextra -pedantic-errors -fno-stack-protector -fno-omit-frame-pointer -fno-builtin -mno-sse -mno-mmx -nostdlib -nostdinc -I.
CPPFLAGS = -m32 -std=c++11 -O2 -D __KERNEL -Wall -Wextra -Wno-overloaded-virtual -Wno-reorder -pedantic-errors -fno-threadsafe-statics -fno-stack-protector -fno-builtin -fno-rtti -fno-exceptions -mno-sse -mno-mmx -nostdinc -nostdlib -I.
LDFLAGS  = -T Makefile.ld

firedrake: $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $^

%.o: %.cpp
	$(CPP) $(CPPFLAGS) -c -o $@ $^

%.o: %.S
	$(CC) $(ASFLAGS) -c -o $@ $^

clean:
	rm $(OBJS)

.PHONY: clean

