## Overview
Firedrake is a very simple kernel for the i486 architecture, with the sole puprose of learning more about low level and system programming. Its designed by an absolute novice to kernel hacking, so it probably includes some stupid/uncommon design decisions! Although there are no plans to make a *nix like system, some of the API follows the POSIX specification and there are plans to widen this out in the future, but a few things might retain their own API which isn't POSIX conform.

Most of the development progress is documented on my [blog](http://widerwille.com/blog/).

## Features
List of note-worthy features:

  * Interrupt handling
  * Memory management (virtual and physical, including a slab allocator for the kernel)
  * Preemptive multitasking
  * An in-kernel dynamic link editor for loading kernel drivers at runtime
  * Support for processes and multithreading
  * Support for loading and executing ELF binaries
  * Support for syscalls
  * Kernel wide unit tests

## Development
Development of Firedrake happens mostly in two branches: The [master](https://github.com/JustSid/Firedrake/tree/master) branch, which contains patches for the current stable release, and the [unstable](https://github.com/JustSid/Firedrake/tree/unstable) branch which contains the changes for the next major release. Versioninging follows the [Semantic Versioning](http://semver.org) standard.

## Compiling
### Prerequisites
You probably have the most succes at compiling Firedrake if you are running some kind Linux flavour with the following packages installed:

  * llvm (at least 3.x)
  * clang (at least 3.x)
  * make

If you want to create mountable ISO files, you also need the following two packages:

  * grub-rescue-pc
  * xorriso

You also need to mark the `/boot/CreateImage.sh` script as executable by running `chmod +x ./boot/CreateImage.sh`.

### Building
To make a complete build of Firedrake, you only need to run `make`, which will compile the kernel as well as all default libraries and programs. If you want to create a bootable ISO file, you need to run the `make install` target. 
You can also run the `help` target which prints a list of targets that are available together with a description of what they do.

## Executing
If you want to run Firedrake but don't want to reboot your PC for it, you can use an emulator. Firedrake has been tested in `qemu` and `VMWare Fusion`, but should also run in `Virtual Box`, `Bochs` and almost every other emulator which emulates a x86 CPU and a PC BIOS.

## License
Firedrake is released under the permissive MIT license. For more informations, please read the [LICENSE](LICENSE.md) file.

### Third party code
Firedrake uses code from the FreeBSD project for 64bit software integer division. The code can be found in /sys/libc/bsd/ and is released under the BSD license. See the [README](sys/libc/bsd/README.md) inside /sys/libc/bsd for more informations.
