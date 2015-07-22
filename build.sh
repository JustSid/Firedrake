#!/bin/bash

# Firedrake build script, to make integration of a remote build server easier
# The whole story is that I use OS X but need Linux to build, so I have a VM that runs Debian. My IDE then
# executes the `remote-build` target to build the kernel

# This should really be done via environment variables
USER="justsid"
HOST="172.16.185.131"
REMOTE_PATH="/mnt/hgfs/Firedrake"

if [ $# -eq 0 ]; then
	ssh "$USER@$HOST" "cd ${REMOTE_PATH}; ./build.sh --build; exit"
    exit 0
fi

function configureCMake {

	mkdir -p build
	cd build

	CMAKE_LINKER="<CMAKE_LINKER> <CMAKE_CXX_LINK_FLAGS> <LINK_FLAGS> <OBJECTS> -o <TARGET> <LINK_LIBRARIES>"
	cmake ${REMOTE_PATH} -DCMAKE_CXX_COMPILER="$(which clang)" -DCMAKE_C_COMPILER="$(which clang)" -DCMAKE_LINKER="$(which ld)" -DCMAKE_CXX_LINK_EXECUTABLE="${CMAKE_LINKER}" -DCMAKE_C_LINK_EXECUTABLE="${CMAKE_LINKER}"

}

if [ "$@" == "--build" ]; then

	configureCMake
	make

	cd .. # configureCMake moves us into the ./build folder

	./initrd.py
	./boot/image.py

	exit 0
fi

if [ "$@" == "--clean" ]; then

	configureCMake
	make clean

	exit 0
fi

if [ "$@" == "--run" ]; then

	BASEDIR=$(dirname $0)

	QEMU_NET="-net nic,model=rtl8139 -net user"
	QEMU_CPU="-cpu core2duo -smp cores=2"
	QEMU_ARGS="${QEMU_CPU} ${QEMU_NET} -serial stdio"

	qemu-system-i386 ${QEMU_ARGS} "$BASEDIR/boot/Firedrake.iso"

	exit 0
fi
