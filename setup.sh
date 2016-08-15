#!/bin/bash

BASEDIR="$(cd "$(dirname "${BASH_SOURCE[0]}" )" && pwd)"

# Symlink all inter dependencies into place

LINKS=("${BASEDIR}/sys/libc" "${BASEDIR}/sys/libcpp" "${BASEDIR}/sys/libio" "${BASEDIR}/slib/libkern/libc" "${BASEDIR}/slib/libkern/libcpp")

for i in ${LINKS[@]}; do
	if [ -L "${i}" ]; then
		rm "${i}"
	fi
done

ln -s "${BASEDIR}/lib/libc" "${BASEDIR}/sys/libc"
ln -s "${BASEDIR}/lib/libcpp" "${BASEDIR}/sys/libcpp"
ln -s "${BASEDIR}/slib/libio" "${BASEDIR}/sys/libio"

ln -s "${BASEDIR}/lib/libc" "${BASEDIR}/slib/libkern/libc"
ln -s "${BASEDIR}/lib/libcpp" "${BASEDIR}/slib/libkern/libcpp"

# Create required folders

if [ ! -d "${BASEDIR}/build" ]; then
	mkdir "${BASEDIR}/build"
fi
