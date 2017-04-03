import sublime, sublime_plugin, subprocess, os, shutil, getpass, socket
from ..utils.Globals import g_CLARA_SETTINGS

def getFromShell(str):
	return subprocess.check_output(str, shell=True).decode('utf-8').strip()

class ClaraInsertDiagnosisCommand(sublime_plugin.TextCommand):

	def __init__(self, view):
		super(ClaraInsertDiagnosisCommand, self).__init__(view)
		self.error_count = 0

	def _command_exists(cmd):
		return shutil.which(cmd) is not None

	def run(self, edit):
		self.edit = edit
		self._diagnose()
		if self.error_count == 0:
			self._print_line('\n', 'Everything seems to be OK!')
		else:
			self._print_line('\n', 'Please fix', str(self.error_count), 
				'error' if self.error_count == 1 else 'errors')

	def _diagnose(self):

		CLANG = "clang++"
		SHELL_CMD1 = "clang++ -E -x c++ - -v < /dev/null 2>&1 | awk '/#include <...> search starts here/{f=1;next} /End of search list./{f=0} f'"
		SHELL_CMD2 = "clang++ -E -x c++ - -v < /dev/null 2>&1 | awk 'BEGIN{FS=\"-resource-dir\"} /-resource-dir/ {print $2}' | awk '{print $1;}'"

		self.error_count = 0
		
		clangBinary = getFromShell('which clang++')
		if clangBinary:
			self._OK('found clang binary at "{}"'.format(clangBinary))
		else:
			self._ERR('clang was NOT found')
			return
		output = subprocess.check_output(SHELL_CMD1, shell=True)
		output = output.decode('utf-8')
		headers = output.splitlines()
		for i, header in enumerate(headers):
			header = header.strip()
			if header.endswith(' (framework directory)'):
				header = header[:-len(' (framework directory)')]
			headers[i] = os.path.abspath(header)
		builtinHeaders = os.path.abspath(getFromShell(SHELL_CMD2))

		self._OK('System headers:')
		for header in headers:
			self._print_line('  ', header)

		self._OK('Builtin headers:', builtinHeaders)

		settings = sublime.load_settings(g_CLARA_SETTINGS)
		key = '{}@{}'.format(getpass.getuser(), socket.gethostname())
		headersdict = settings.get(key, None)
		if headersdict:
			self._OK(key, 'is present in settings.')
		else:
			self._ERR('NO headers found for', key)
		if isinstance(headersdict['system_headers'], list):
			self._OK('system_headers key is present.')
		else:
			self._ERR('Missing system_headers key!')
			self._print_line('You should run the command "Clara: Generate System Headers" from the command palette.')
		if isinstance(headersdict['system_frameworks'], list):
			self._OK('system_frameworks key is present.')
		else:
			self._ERR('Missing system_frameworks key!')
			self._print_line('You should run the command "Clara: Generate System Headers" from the command palette.')
		project = self.view.window().project_data()
		project_file_name = self.view.window().project_file_name()
		if project_file_name:
			self._OK('Found project', project_file_name)
		else:
			self._ERR('Did NOT find a sublime-project file.')
			self._print_line('You should open or create a sublime-project file.')
		settings = self.view.settings()
		compile_commands = settings.get('compile_commands', None)
		if compile_commands:
			compile_commands = sublime.expand_variables(compile_commands, self.view.window().extract_variables())
			self._OK('Found "compile_commmands":', compile_commands)
			if os.path.isdir(compile_commands):
				self._OK(compile_commands, 'is a directory.')
			else:
				self._ERR(compile_commands, 'is NOT a directory.')
				self._print_line('Make sure you have entered the directory where your build artifacts are created.')
			path = os.path.join(compile_commands, 'compile_commands.json')
			if os.path.isfile(path):
				self._OK(path, 'is a file.')
			else:
				self._ERR(path, 'is NOT a file.')
				self._print_line('Make sure you export your compilation commands with your build system.')
		else:
			self._ERR('No "compile_commands" settings found in ' + project_file_name)
			self._print_line('Please provide a "compile_commands" setting in the "settings" dictionary of your sublime-project file.')
			self._print_line('The value should be the directory where compile_commands.json lives.')
			return

	def _print_line(self, *entries):
		self.view.insert(self.edit, self.view.size(), ' '.join(entries) + '\n')

	def _OK(self, *entries):
		self._print_line('\u2705', *entries)

	def _ERR(self, *entries):
		self._print_line('\u274C', *entries)
		self.error_count += 1
