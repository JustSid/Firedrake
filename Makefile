KERNEL_DIR  = sys
LIBS_DIR    = lib
LIBKERN_DIR = libkernel
PROGS_DIR   = bin

ALLDIRS = $(KERNEL_DIR) $(LIBKERN_DIR) $(LIBS_DIR) $(PROGS_DIR)

all:
	for i in $(ALLDIRS); do make -C $$i; done

install: all
	./boot/CreateImage.sh

kernel:
	make -C $(KERNEL_DIR)

libraries:
	make all -C $(LIBS_DIR)

driver:
	make all -C $(LIBKERN_DIR)

programs:
	make all -C $(PROGS_DIR)

clean:
	for i in $(ALLDIRS); do make clean -C $$i; done

help:
	@echo "List of valid targets:"
	@echo "	-all (default configuration, compiles everything)"
	@echo "	-install (like all, but also creates a mountable iso with Firedrake)"
	@echo "	-kernel (builds the kernel)"
	@echo "	-libraries (builds all libraries)"
	@echo "	-driver (builds all drivers and the driver framework libio)"
	@echo " -programs (builds all programs)"

.PHONY: clean
.PHONY: libraries
.PHONY: driver
.PHONY: programs
.PHONY: help
