from .Clara import *
import os, subprocess

SHELL_CMD1 = "clang++ -E -x c++ - -v < /dev/null 2>&1 | awk '/#include <...> search starts here/{f=1;next} /End of search list./{f=0} f'"
SHELL_CMD2 = "clang++ -E -x c++ - -v < /dev/null 2>&1 | awk 'BEGIN{FS=\"-resource-dir\"} /-resource-dir/ {print $2}' | awk '{print $1;}'"
FILENAME = '/home/raoul/.config/sublime-text-3/Packages/Clara/test/hello.cpp'

output = subprocess.check_output(SHELL_CMD1, shell=True)
output = output.decode('utf-8')
headers = output.splitlines()
for i, header in enumerate(headers):
	header = header.strip()
	if header.endswith(' (framework directory)'):
		header = header[:-len(' (framework directory)')]
	headers[i] = os.path.abspath(header)
output = subprocess.check_output(SHELL_CMD2, shell=True)

options = SessionOptions()
options.systemHeaders = headers
options.builtinHeaders = os.path.abspath(output.decode('utf-8').strip())
options.filename = FILENAME

session = Session(options)

results = None
with open(FILENAME) as f:
	results = session.codeComplete(f.read(), 50, 1)
print(results)
