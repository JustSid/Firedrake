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

#### Libraries
 * **libtest: Added** fork(), mmap() and munmap() implementations
 * **libtest: Added** the new pid_t type for pids