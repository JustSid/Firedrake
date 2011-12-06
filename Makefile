CC  = clang
LD  = llvm-link

KERNEL_DIR = Kernel
ALLDIRS = $(KERNEL_DIR)

all:
	for i in $(ALLDIRS); do make -C $$i; done

kernel:
	make -c $(KERNEL_DIR);

install:
	./Bootable/CreateImage.sh

clean:
	for i in $(ALLDIRS); do make clean -C $$i; done

.PHONY: clean
