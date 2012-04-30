##Getting started
Before you can build Firedrake, you need the following packages:

  * grub-rescue-pc
  * xorriso
  * llvm
  * clang

The next thing you need to do is to chmod the Image generator script, cd into /Bootable and run `chmod +x ./CreateImage.sh`

##Compiling Firedrake
cd into the Firedrake directory (the one which includes THIS file, ya know?) and then run

	make
	make install

The `make install` is only needed if you want to create a bootable .iso file, if you only need the kernel binary you can simply use `make` and grab it from the kernels source folder.