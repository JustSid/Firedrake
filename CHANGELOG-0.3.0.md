## Firedrake 0.3.0:768 (Jormungand)
### Overview
Firedrake 0.3.0 focuses mainly on the kernels runtime link editor which allows the kernel to load dynamic libraries into the kernelspace which can then extend the kernel functionality. There is also a huge amount of fixes and tweaks which make Firedrake 0.3.0 much faster and more stable than its predecessors.

### Changes
#### Kernel
* **Added** ioglue which can load dynamic ELF libraries into the kernel space
* **Added** an array, andersson tree, hash table and ringbuffer container type
* **Added** ab iterator type which is able to iterate over container
* **Added** time keeping and tracking functionality
* **Added** interrupt 0x31 can be used to yield the current thread
* **Added** the kernels backtracing function can symbolicated C++ functions
* **Added** you can now pass command line arguments to the kernel at boot time. Possible arguments are `--debug`, `--dumpgrub` and `--dumppci`
* **Removed** interrupt 0x30 can't be used for syscalls anymore, use 0x80 instead!
* **Changed** dylink is now called loader (together with a new prefix, `ld`)
* **Changed** the heap allocator is now much more intelligent and less wasteful when it comes to small allocations
* **Changed** spinlocks now yield the current thread when they can't obtain the lock
* **Changed** the panic function now dumps the state of the processer when an exception occured
* **Changed** the backtrace function now displays the unresolved return address of the function for easier debugging
* **Fixed** strcpy() didn't append a null byte to the string

#### Libkernel
This is the first release which includes libkernel, a framework that allows the development of drivers that can be loaded at runtime into the kernel (linked using ioglue). At the moment there are four libraries (two essential kernel libraries and two test libraries), `libkernel` which is a C written library that exports functionality from the kernel (interrupt handling, thread creation etc), `libio` which adds further abstraction to libkernel by implementing various C++ classes and containers. `libPCI` is a test library used to test libio, it enumerates over PCI devices on the bus and uses libio's service matching to create controller classes for the found devices. `libRTL8139` is a very simple implementation of a PCI device controller for the Realteak 8139 ethernet controller.

There is also some work in progress documentation for [libio available](http://widerwille.com/firedrake/docs/latest/libio/).

### Screenshots
![a booted Firedrake 0.3](http://widerwille.com/firedrake/firedrake14.png)
![the new panic screen](http://widerwille.com/firedrake/firedrake15.png)