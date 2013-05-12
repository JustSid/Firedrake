#!/usr/bin/python

import os
import shutil
import subprocess

base = os.path.dirname(os.path.realpath(__file__))

def createDir(path):
	path = os.path.join(base, path)

	if not os.path.exists(path):
		os.makedirs(path)

def deleteDir(path):
	path = os.path.join(base, path)
	shutil.rmtree(path)

def copyFile(source, target):
	source = os.path.join(base, source)
	target = os.path.join(base, target)

	shutil.copyfile(source, target)


createDir('image')
createDir('image/boot/grub/grup-i386-pc')
createDir('image/modules')

copyFile('grub.cfg', 'image/boot/grub/grub.cfg')
copyFile('../sys/firedrake', 'image/boot/firedrake')
copyFile('initrd', 'image/modules/initrd')

os.chdir(base)
os.system('grub-mkrescue --modules="ext2 fshelp boot pc" --output="./Firedrake.iso" "./image"')

deleteDir('image')
