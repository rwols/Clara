import sublime, sublime_plugin, subprocess, os

def getFromShell(str):
	return subprocess.check_output(str, shell=True).decode('utf-8').strip()

class ClaraInsertDiagnosisCommand(sublime_plugin.TextCommand):

	def __init__(self, view):
		super(ClaraInsertDiagnosisCommand, self).__init__(view)
		self.errorCount = 0

	def run(self, edit):
		self._diagnose(edit)
		if self.errorCount == 0:
			self._printLine(edit, '\nEverything seems to be OK!')

	def _diagnose(self, edit):

		CLANG = "clang++"
		SHELL_CMD1 = "clang++ -E -x c++ - -v < /dev/null 2>&1 | awk '/#include <...> search starts here/{f=1;next} /End of search list./{f=0} f'"
		SHELL_CMD2 = "clang++ -E -x c++ - -v < /dev/null 2>&1 | awk 'BEGIN{FS=\"-resource-dir\"} /-resource-dir/ {print $2}' | awk '{print $1;}'"

		self.errorCount = 0
		
		clangBinary = getFromShell('which clang++')
		if clangBinary:
			self._OK(edit, 'found clang binary at "{}"'.format(clangBinary))
		else:
			self._ERR(edit, 'clang was NOT found')
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

		self._OK(edit, 'System headers:')
		for header in headers:
			self._printLine(edit, '\t' + header)

		self._OK(edit, 'Builtin headers: ' + builtinHeaders)

		settings = sublime.load_settings('Clara.sublime-settings')
		if settings.get('system_headers'):
			self._OK(edit, 'System headers are present in User settings.')
		else:
			self._ERR(edit, 'System headers are NOT set in User settings. Please run the command "Generate System Headers" from the command palette to fix this.')
		if settings.get('builtin_headers'):
			self._OK(edit, 'Builtin headers are present in User settings.')
		else:
			self._ERR(edit, 'Builtin headers are NOT set in User settings. Please run the command "Generate System Headers" from the command palette to fix this.')

		project = self.view.window().project_data()
		projectFilename = self.view.window().project_file_name()

		if projectFilename:
			self._OK(edit, 'Found project "{}"'.format(projectFilename))
		else:
			self._ERR(edit, 'Did NOT find a sublime-project file.')
			return

		if not project:
			self._ERR(edit, 'Could not open file "{}"'.format(projectFilename))
			return

		cmakeSettings = project['cmake']

		if cmakeSettings:
			cmakeSettings = sublime.expand_variables(cmakeSettings, self.view.window().extract_variables())
			# cmakeFile = cmakeSettings['cmake_file']
			buildFolder = cmakeSettings['build_folder']
			# if cmakeFile:
			# 	self._OK(edit, 'Found CMake file: ' + cmakeFile)
			# else:
			# 	self._ERR(edit, 'No cmake_file present in clara settings of ' + projectFilename)
			if buildFolder:
				self._OK(edit, 'Found CMake build folder "{}"'.format(buildFolder))

				COMPILE_COMMANDS = 'compile_commands.json'
				compileCommands = os.path.join(buildFolder, COMPILE_COMMANDS)
				if os.path.isfile(compileCommands):
					self._OK(edit, 'Found {} in {}'.format(COMPILE_COMMANDS, buildFolder))
				else:
					self._ERR(edit, 'Did NOT find "{}" in the build folder "{}".\n\
Clara needs this file to know exactly which compiler arguments are passed to \
each individual source file for correct auto-completion. \
Make sure to have the line "set(CMAKE_EXPORT_COMPILE_COMMANDS ON)" in \
your top-level CMakeLists.txt file, or pass it as a command line argument \
to your cmake invocation. Alternatively, perhaps you still need to configure \
your cmake project.'.format(COMPILE_COMMANDS, buildFolder))
					return

				os.path.isfile(os.path.join(buildFolder, "compile_commands.json"));

			else:
				self._ERR(edit, 'No build_folder present in clara settings of ' + projectFilename)
		else:
			self._ERR(edit, 'No cmake settings found in ' + projectFilename)
			return

	def _printLine(self, edit, str):
		self.view.insert(edit, self.view.size(), str + '\n')

	def _OK(self, edit, str):
		self._printLine(edit, '\n\u2705 ' + str)

	def _ERR(self, edit, str):
		self._printLine(edit, '\n\u274C ' + str)
		self.errorCount += 1
