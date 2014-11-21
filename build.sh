#!/bin/bash
# Firedrake build script
# Set USESSH to true to build Firedrake remotely using SSH, otherwise it will build Firedrake on the local machine
# Note though that running Firedrake via qemu always happens on the local machine

USESSH=true

# Settings for the remote host
USER="justsid"
HOST="192.168.178.38"
REMOTE_PATH="/mnt/hgfs/Firedrake"

# Settings for the local host
BASEDIR=$(dirname $0)
QEMU_NET="-net nic,model=rtl8139 -net user"
QEMU_CPU="-cpu core2duo -smp cores=2"
QEMU_ARGS="$QEMU_CPU $QEMU_NET -serial stdio"

if [ $# -eq 0 ]; then
	if $USESSH ; then
    	ssh "$USER@$HOST" "cd $REMOTE_PATH; make install; exit"
	else
		make install
	fi
    exit 0
fi

if [ "$@" == "--clean" ]; then
	if $USESSH ; then
		ssh "$USER@$HOST" "cd $REMOTE_PATH; make clean; exit"
	else
		make clean
	fi
	exit 0
fi

if [ "$@" == "--run" ]; then
	qemu-system-i386 $QEMU_ARGS "$BASEDIR/boot/Firedrake.iso"
	exit 0
fi

if [ "$@" == "--debug" ]; then
	qemu-system-i386 -d int -D "/tmp/qemu.log" -no-reboot -no-shutdown $QEMU_ARGS "$BASEDIR/boot/Firedrake.iso"
	exit 0
fi

if [ "$@" == "--debug-gdb" ]; then
	qemu-system-i386 -d int -D "/tmp/qemu.log" -no-reboot -no-shutdown -s -S $QEMU_ARGS "$BASEDIR/boot/Firedrake.iso"
	exit 0
fi

echo "No argument to build, --clean to clean all targets, --run to start in qemu and --debug to start in qemu with additional debug flags!"
