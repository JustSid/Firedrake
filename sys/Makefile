include ../Makefile.rules

SRCS = $(shell find $(CURDIR) -type f \( -name "*.cpp" -o -name "*.c" -o -name "*.S" $(CONFIG_EXCLUDE_FOLDERS) \))
OBJS = $(addsuffix .o, $(basename $(SRCS)))

ASFLAGS  = $(CONFIG_TARGET) $(CONFIG_OPTIMIZATION) -D __KERNEL -I.
CFLAGS 	 = $(CONFIG_TARGET) $(CONFIG_OPTIMIZATION) -std=c99 -O1 -D __KERNEL -Wall -Wextra -pedantic-errors -fno-stack-protector -fno-omit-frame-pointer -fno-builtin -mno-sse -mno-mmx -nostdlib -nostdinc -I.
CPPFLAGS = $(CONFIG_TARGET) $(CONFIG_OPTIMIZATION) -std=c++11 -O1 -D __KERNEL -Wall -Wextra -Wno-overloaded-virtual -Wno-reorder -pedantic-errors -fno-threadsafe-statics -fno-stack-protector -fno-builtin -fno-rtti -fno-exceptions -mno-sse -mno-mmx -nostdinc -nostdlib -I.
LDFLAGS  = -T Makefile.ld 

firedrake: $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $^ $(CONFIG_BUILD_LIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $^

%.o: %.cpp
	$(CPP) $(CPPFLAGS) -c -o $@ $^

%.o: %.S
	$(AS) $(ASFLAGS) -c -o $@ $^

clean:
	rm $(OBJS)

.PHONY: clean

