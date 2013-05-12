#!/usr/bin/python

import os
import struct

def appendFile(out, filepath, path):
	f = open(filepath, 'rb')
	b = f.read()
	f.close()

	path = os.path.join(path, os.path.basename(filepath))

	out.write(struct.pack('<II%is' %(len(path)), len(path), len(b), path.encode('ascii', 'replace')))
	out.write(b)

def scanFolder(out, path):
	for filename in os.listdir(path):
		if filename == '.git':
			continue

		filepath = os.path.join(path, filename)

		if filename.rfind('.bin') >= 0:
			appendFile(out, filepath, '/bin')
		elif filename.rfind('.so') >= 0:
			appendFile(out, filepath, '/lib')
		elif filename == 'firedrake':
			appendFile(out, filepath, '/')

		if os.path.isdir(filepath) == True:
			scanFolder(out, filepath)

def appendFolder(out, path, folder):
	for filename in os.listdir(path):
		filepath = os.path.join(path, filename)
		
		if os.path.isfile(filepath) == True:
			appendFile(out, filepath, folder)

		if os.path.isdir(filepath) == True:
			afolder = os.path.join(folder, filename)
			appendFolder(out, filepath, afolder)
		

directory = os.path.dirname(os.path.realpath(__file__))
out = open(os.path.join(directory, 'boot/initrd'), 'wb')

scanFolder(out, directory)
appendFolder(out, os.path.join(directory, 'etc'), '/etc')

out.close()