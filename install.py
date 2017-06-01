#!/usr/bin/env python

import tempfile, os, subprocess, multiprocessing, sys

def execute(cwd, cmd):
	popen = subprocess.Popen(cmd, stdout=subprocess.PIPE, 
		universal_newlines=True, shell=True, cwd=cwd)

	for stdout_line in iter(popen.stdout.readline, ""):
		yield stdout_line

	popen.stdout.close()
	return_code = popen.wait()

	if return_code:
		raise subprocess.CalledProcessError(return_code, cmd)

def execute2(cwd, cmd):
	for line in execute(cwd, cmd):
		sys.stdout.write(line)

def big_print(message):
	print('\n\t{}\n'.format(message))

def install():
	tempdir = tempfile.gettempdir()
	big_print('Working in temporary folder "{}"'.format(tempdir))
	builddir = os.path.join(tempdir, 'llvm-build')
	llvmsourcedir = os.path.join(tempdir, 'llvm')
	llvmtoolsdir = os.path.join(llvmsourcedir, 'tools')
	clangtoolsdir = os.path.join(llvmtoolsdir, 'clang', 'tools')

	big_print('Cloning LLVM sources...')
	execute2(tempdir, 'svn co http://llvm.org/svn/llvm-project/llvm/trunk llvm')

	big_print('Cloning Clang sources...')
	execute2(llvmtoolsdir, 'svn co http://llvm.org/svn/llvm-project/llvm/trunk clang')

	big_print('Cloning Clara sources...')
	execute2(clangtoolsdir, 'git clone --recursive https://github.com/rwols/Clara.git')

	big_print('Configuring with CMake in "{}"'.format(builddir))
	execute2(tempdir, 'cmake -Hllvm -Bllvm-build -DLLVM_ENABLE_RTTI=ON -DLLVM_ENABLE_EH=ON -DCLANG_RESOURCE_DIR=. -DCMAKE_BUILD_TYPE=Release')

	cpu_count = str(multiprocessing.cpu_count())
	big_print('Building with {} jobs'.format(cpu_count))
	execute2(builddir, 'make ClaraInstall -j{}'.format(cpu_count))

	big_print('Clara is installed!')

def main():
	try:
		install()
	except Exception as e:
		print(e)

if __name__ == '__main__':
	main()
