CC  = clang
LD  = llvm-link

KERNEL_DIR = Kernel
ALLDIRS = $(KERNEL_DIR)

all:
	for i in $(ALLDIRS); do make -C $$i; done

debug: all
	rm -f ./dump.txt
	objdump -d ./Kernel/firedrake >> ./dump.txt

kernel:
	make -c $(KERNEL_DIR);

install: all
	./Bootable/CreateImage.sh

clean:
	for i in $(ALLDIRS); do make clean -C $$i; done

.PHONY: clean
