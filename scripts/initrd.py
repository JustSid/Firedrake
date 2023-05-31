#!/usr/bin/python3

import os
import struct

def appendFile(out, filepath, path):
	f = open(filepath, 'rb')
	b = f.read()
	f.close()

	path = os.path.join(path, os.path.basename(filepath))

	out.write(struct.pack('<II%is' %(len(path)), len(path), len(b), path.encode('ascii', 'replace')))
	out.write(b)

def scanFolder(out, path, target):
	for filename in os.listdir(path):
		if filename == '.git':
			continue

		filepath = os.path.join(path, filename)

		if filename.endswith('.bin') or filename.endswith('.so') or filename == 'firedrake':
			appendFile(out, filepath, target)

		if os.path.isdir(filepath) == True:
			scanFolder(out, filepath, target)

def appendFolder(out, path, folder):
	for filename in os.listdir(path):
		filepath = os.path.join(path, filename)
		
		if os.path.isfile(filepath) == True:
			appendFile(out, filepath, folder)

		if os.path.isdir(filepath) == True:
			afolder = os.path.join(folder, filename)
			appendFolder(out, filepath, afolder)
		

directory = os.path.dirname(os.path.realpath(__file__))
out = open(os.path.join(directory, '../boot/initrd'), 'wb')

scanFolder(out, os.path.join(directory, '../build/bin'), '/bin')
scanFolder(out, os.path.join(directory, '../build/lib'), '/lib')
scanFolder(out, os.path.join(directory, '../build/slib'), '/slib')
scanFolder(out, os.path.join(directory, '../build/sys'), '/')
appendFolder(out, os.path.join(directory, '../etc'), '/etc')

out.close()