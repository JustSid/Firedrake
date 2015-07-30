#!/bin/bash

BASEDIR=$(dirname ${0})

# Symlink all inter dependencies into palce

if [ -d "${BASEDIR}/sys/libc" ]; then
	rm "${BASEDIR}/sys/libc"
fi
if [ -d "${BASEDIR}/sys/libio" ]; then
	rm "${BASEDIR}/sys/libio"
fi


if [ -d "${BASEDIR}/slib/libkern/libc" ]; then
	rm "${BASEDIR}/slib/libkern/libc"
fi


ln -s "${BASEDIR}/lib/libc" "${BASEDIR}/sys/libc"
ln -s "${BASEDIR}/lib/libc" "${BASEDIR}/slib/libkern/libc"

ln -s "${BASEDIR}/slib/libio/core" "${BASEDIR}/sys/libio"

# Create required folders

if [! -d "${BASEDIR}/build" ]; then
	mkdir "${BASEDIR}/build"
fi
