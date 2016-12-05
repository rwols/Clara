#!/usr/bin/env python3
import sys, subprocess, os
sys.path.append('../../../../../build/lib') # WARNING: system dependent...
from Clara import *

def fetchSystemHeaders():
	SHELL_CMD = "clang++ -E -x c++ - -v < /dev/null 2>&1 | awk '/#include <...> search starts here/{f=1;next} /End of search list./{f=0} f'"
	output = subprocess.check_output(SHELL_CMD, shell=True)
	output = output.decode('utf-8')
	headers = output.splitlines()
	for i, header in enumerate(headers):
		header = header.strip()
		if header.endswith(' (framework directory)'):
			header = header[:-len(' (framework directory)')]
		headers[i] = os.path.abspath(header)
	return headers

def fetchBuiltinHeaders():
	SHELL_CMD = "clang++ -E -x c++ - -v < /dev/null 2>&1 | awk 'BEGIN{FS=\"-resource-dir\"} /-resource-dir/ {print $2}' | awk '{print $1;}'"
	output = subprocess.check_output(SHELL_CMD, shell=True)
	return os.path.abspath(output.decode('utf-8').strip())

def main():
	if len(sys.argv) < 4:
		print('Usage: {} {} {} {}'.format(sys.argv[0], '<filename>', '<row>', '<column>'))
		exit()
	options = SessionOptions()
	options.builtinHeaders = fetchBuiltinHeaders()
	options.systemHeaders = fetchSystemHeaders()
	options.filename = sys.argv[1]
	options.jsonCompileCommands = '../../../../../build'
	options.logCallback = print
	contents = None
	with open(sys.argv[1], 'r') as file:
		contents = file.read()
	ses = Session(options)
	results = ses.codeComplete(contents, int(sys.argv[2]), int(sys.argv[3]))
	print(results)

if __name__ == '__main__':
	main()