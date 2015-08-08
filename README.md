# Overview

Firedrake is my little kernel project. It’s nothing fancy, but it runs on bare metal so suck it! It’s pretty much my go to hobby project when I feel like tinkering with low level stuff, so don’t expect any coherent development or anything serious: This is a just for fun project and I work on whatever seems fun at the time. That being said, the goal is to get a somewhat working OS together, even if it’s just a text based shell.

The project has gone through many iterations, refactorings and rewrites. It started out as pure C project and is now a C++11 based kernel with a simple C interface into the userland. This probably isn’t the end of the refactoring/rewrite process, as I love to throw old stuff away and replace it by newer and better things. So, the message clearly is: *here be dragons*.

## Features

Every feature here is implemented in a *very* simply and rudimentary form! If it sounds impressive, I assure you, it’s not!

* Virtual and physical memory management
* Pre-emptive multitasking
* SMP support
* Virtual file system
* IPC
* ELF executable loader
* System call interface

Of these features, all of them need work to improve them! For example, the ELF loader can’t handle dynamically linked libraries right now, and the scheduler isn’t SMP aware and likes to move threads around CPUs.

Additionally, there is no video output. Everything is done through the UART, so if you throw it at bare metal, make sure to read the UART, otherwise you will miss most of the fun.

# Building Firedrake

Building Firedrake requires a linux machine and some common binaries. If you use the Sublime Text project, you can simply use `cmd + b`, or your local equivalent, to build the project. Otherwise, the `build.sh` file contains everything needed to build it and it can be invoked directly.

However, before you can build, you need to create a bunch of symlinks, which can be done by running `setup.sh`.

* LLVM/Clang 3.5
* binutils
* xorriso
* grub-mkrescue

If the build is successful, the `boot` folder will contain a `Firedrake.iso` file which can be either thrown against a Virtual Machine or bare metal.