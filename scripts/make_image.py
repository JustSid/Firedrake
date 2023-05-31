#!/usr/bin/python3

import os
import shutil

base = os.path.dirname(os.path.realpath(__file__))
base = os.path.join(base, '../boot')


def create_dir(path):
    path = os.path.join(base, path)

    if not os.path.exists(path):
        os.makedirs(path)


def delete_dir(path):
    path = os.path.join(base, path)
    shutil.rmtree(path)


def copy_file(source, target):
    source = os.path.join(base, source)
    target = os.path.join(base, target)

    shutil.copyfile(source, target)


create_dir('image')
create_dir('image/boot/grub/grup-i386-pc')
create_dir('image/modules')

copy_file('grub.cfg', 'image/boot/grub/grub.cfg')
copy_file('initrd', 'image/modules/initrd')
copy_file('../build/sys/firedrake', 'image/boot/firedrake')

os.chdir(base)
os.system('grub-mkrescue --modules="boot" --output="./Firedrake.iso" "./image"')

delete_dir('image')