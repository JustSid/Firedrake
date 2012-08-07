## Firedrake 0.2.1:513 (Sunrise Jade)
### Overview
This update contains a few small changes and additions as well as bugfixes. The probably biggest change is the new folder structure based on FHS.

### Changes
#### Kernel
 * **Added** the fork() syscall
 * **Added** locking for memory related functions (physical memory, virtual memory, kernel allocator and mappings)
 * **Added** a new type, pid_t used to store pids. Its currently a signed 32bit integer
 * **Changed** the folder structure of Firedrake based on FHS
 * **Fixed** mmap() and munmap() are now callable
 * **Fixed** ring0 processes now spawn threads with hypervisor priviliges

### Libraries
 * **libtest: Added** fork(), mmap() and munmap() implementations
 * **libtest: Added** the new pid_t type for pids


## Firedrake 0.2.0:512 (Sunrise Jade)
### Overview
Firedrake 0.2.0 changes a lot of things under the hood, mainly related to the handling of memory and the execution of processes and the handling of their syscalls.
Unlike previous versions, Firedrake now gives each process its own 4gb address space and handles interrupts via a trampoline area in the memory space which is responsible for loading the kernels address space and call the interrupt handler there, this is the only part of the kernel which is mapped into a process' address space, otherwise the whole 4gb are reserved for the process.

The kernel can also load ELF binaries from multiboot modules now, as long as they are selfcontained executables. Dynamic libraries are not resolved and no relocation is done by the kernel or a dynamic linker. Spinlocks are also reintroduced, although not completely covering all possible areas (eg. the memory related stuff is not yet threadsafe!)

### Changes
#### Kernel
 * **Added** dylink which is able to load ELF binaries
 * **Added** the possibility for the kernel to create processes from ELF binaries
 * **Added** a completly rewritten interrupt handler
 * **Added** kunit which is able to run in kernel unit-tests
 * **Added** config.h which can be used to configure various stuff (eg. the exectuion of unit-tests, force to not-inline some functions for better stacktraces etc)
 * **Added** stacktraces for the kernel which enables the kernel to dump the callstack in case of errors
 * **Added** various stuff to the kernels libc subset, most notably setjmp/longjmp, strcmp, strstr, strcpy, strncmp and strpbrk
 * **Added** a high level memory allocator for the kernel which allows for smaller allocations than the previous kalloc() which always allocated at least one whole page
 * **Added** new logging macros, dbg, err, warn and info which call syslog() with LOG_DEBUG, LOG_ERROR, LOG_WARNING, and LOG_INFO respectively
 * **Changed** `vsnprintf()` now accepts flag and width arguments in the format string
 * **Changed** the order of boot module initialization, previously the interrupt handler was loaded before the memory management system, this is now changed
 * **Changed** the colors for LOG_ERROR and LOG_WARNING are switched now
 * **Changed** structs now use the `_s` suffix and only typedeffed stuff use the `_t` suffix
 * **Changed** `vm_init()` now also maps the multiboot info, except of the memory mapping information!
 * **Changed** `vm_offset_t` is now called `vm_address_t`
 * **Changed** `vm_getPhysicalAddress()` is now named `vm_resolveVirtualAddress()`
 * **Changed** syscalls now return an errno value
 * **Fixed** various virtual memory related functions which crashed because they accessed unmapped memory
 * **Fixed** a bug in `atof()` which ignored numbers after the decimal seperator.
 * **Fixed** the multiboot info and the multiboot modules are now marked as used by the physical memory manager, so they are no longer overwritten at runtime.
 * **Removed** `ceil`, `ceilf`, `fabs` and `dabs` from math.h (the goal was to remove all floating point code from the kernel).

#### Libraries
 * **Note** this is the first release of Firedrake which includes libraries. Currently only static libraries are supported because of the lack of an dynamic linker
 * **Added** libcrt, usually this is just an .o file, however currently its provided as an .a static library, it only implements _start() which then calls the main() function of the actual program
 * **Added** libtest, static test library which just implements C routines for most syscalls and also implements some other stuff like errno

#### Programs
 * **Note** this is the first release of Firedrake which includes programs.
 * **Added** hellostatic, a simple program which puts a message on the screen, creates a secondary thread and then joins the thread.

### Notes
 * Firedrake now perfectly compiles and links using LLVM/Clang 3.1, it also uses -O2 optimization and thus is much faster and lighter than previous versions.
 * This release has been tested on Qemu and VMWare Fusion 4.1.3

### Screenshots
![the new backtraces](http://widerwille.com/firedrake/firedrake04.png)
![a program running](http://widerwille.com/firedrake/firedrake05.png)
