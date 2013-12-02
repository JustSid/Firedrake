include ../Makefile.rules

SRCS = $(shell find $(CURDIR) -type f \( -name "*.cpp" -o -name "*.c" -o -name "*.S" \))
OBJS = $(addsuffix .o, $(basename $(SRCS)))

ASFLAGS  = -m32 -I. -no-integrated-as
CFLAGS 	 = -m32 -std=c99 -Wall -Wextra -mno-sse -mno-mmx -nostdlib -nostdinc -fno-stack-protector -fno-omit-frame-pointer -fno-builtin -O2 -pedantic-errors -I.
CPPFLAGS = -m32 -std=c++11 -Wall -Wextra -mno-sse -mno-mmx -Wno-overloaded-virtual -Wno-reorder -pedantic-errors -O2 -fno-threadsafe-statics -fno-stack-protector -fno-builtin -fno-rtti -fno-exceptions -nostdinc -nostdlib -I.
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

