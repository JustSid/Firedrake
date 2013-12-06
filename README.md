## Overview
This branch contains the complete rewrite of Firedrake, the goal is a more "modern" kernel which goes away from the classical UNIX approach. Since Firedrake is in no way anything more than a simple hobby, I decided that it would be best to try out things on my own instead of just copying what everyone else did.

The kernel itself is written in a crude mix between C and C++ and there is definitely more potential to rewrite parts to be more real C++ and less legacy C (this is partially due to the fact that some code simply got taken from the original Firedrake source and because the libc++ subset implemented is severly lacking).

### Bucket list
These are the things I want to implement in Firedrake in the next time, in no particular order

 * IPC
 * VFS
 * ELF loader
 * Runtime link editor in the kernel
 * Driver framework
 * Full ACPI support

### Already implemented
Some notworthy things that got already implemented in Firedrake

 * Memory management (physical, virtual, slab allocator)
 * Interrupts through APIC(s) and I/O APIC(s)
 * Basic threading and task support (including a simple scheduler)
 * SMP (ACPI is used to read out all CPUs and they are already started and take part in the scheduling process)

## Building
Not much has changed for building Firedrake. Same tools, same procedure, same build scripts.

## License
Firedrake is released under the permissive MIT license. For more informations, please read the [LICENSE](LICENSE.md) file.

### Third party code
Firedrake uses code from the FreeBSD project for 64bit software integer division. The code can be found in /sys/libc/bsd/ and is released under the BSD license. See the [README](sys/libc/bsd/README.md) inside /sys/libc/bsd for more informations.
