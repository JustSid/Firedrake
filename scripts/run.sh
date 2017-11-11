#!/bin/bash

BASEDIR=$(dirname $0)

QEMU_NET="-net nic,model=rtl8139 -net user"
QEMU_CPU="-cpu core2duo -smp cores=2"
QEMU_BOOTDRIVE="-drive format=raw,media=cdrom,readonly,file=${BASEDIR}/../boot/Firedrake.iso"

QEMU_ARGS="${QEMU_CPU} ${QEMU_NET} -serial stdio"


if [ "${1}" == "--debug" ]; then

	qemu-system-x86_64 ${QEMU_ARGS} -s -S -D /tmp/qemu.log -d int -no-shutdown -no-reboot ${QEMU_BOOTDRIVE} &
	exit 0

fi

qemu-system-x86_64 ${QEMU_ARGS} ${QEMU_BOOTDRIVE}
exit 0
