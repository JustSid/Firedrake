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
	qemu-system-i386 $QEMU_NET "$BASEDIR/boot/Firedrake.iso"
	exit 0
fi

if [ "$@" == "--debug" ]; then
	qemu-system-i386 -d int,cpu_reset -no-reboot -no-shutdown $QEMU_NET "$BASEDIR/boot/Firedrake.iso"
	exit 0
fi

echo "No argument to build, --clean to clean all targets, --run to start in qemu and --debug to start in qemu with additional debug flags!"
