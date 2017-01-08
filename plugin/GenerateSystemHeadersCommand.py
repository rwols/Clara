import sublime, sublime_plugin, subprocess, os

class ClaraGenerateSystemHeadersCommand(sublime_plugin.ApplicationCommand):
	"""Generates system header info."""

	def run(self):
		
		SHELL_CMD1 = """clang++ -E -x c++ - -v < /dev/null 2>&1 | awk '/#include <...> search starts here/{f=1;next} /End of search list./{f=0} f'"""
		SHELL_CMD2 = """clang++ -E -x c++ - -v < /dev/null 2>&1 | awk 'BEGIN{FS="-resource-dir"} /-resource-dir/ {print $2}' | awk '{print $1;}'"""
		USER_MSG = 'System headers have been saved in user settings. Do you want to view them now?'
		SETTINGS = 'Clara.sublime-settings'
		ENDS_WITH_FRAMEWORK = ' (framework directory)'

		output = subprocess.check_output(SHELL_CMD1, shell=True)
		output = output.decode('utf-8')
		raw_headers = output.splitlines()
		frameworks = []
		headers = []
		for raw_header in raw_headers:
			raw_header = raw_header.strip()
			if raw_header.endswith(ENDS_WITH_FRAMEWORK):
				raw_header = raw_header[:-len(ENDS_WITH_FRAMEWORK)]
				frameworks.append(os.path.abspath(raw_header))
			else:
				headers.append(os.path.abspath(raw_header))
		output = subprocess.check_output(SHELL_CMD2, shell=True)
		resource_dir = os.path.abspath(output.decode('utf-8').strip())
		builtin_include = os.path.join(resource_dir, 'include')
		if builtin_include in headers: headers.remove(builtin_include)
		packages_path = sublime.packages_path()
		resource_dir = os.path.join(packages_path, 'Clara', 'include')
		settings = sublime.load_settings(SETTINGS)
		settings.set('builtin_headers', resource_dir)
		settings.set('system_headers', headers)
		settings.set('system_frameworks', frameworks)
		sublime.save_settings(SETTINGS)
		if sublime.ok_cancel_dialog(USER_MSG):
			sublime.run_command('open_clara_settings')

	def description(self):
		return 'Generates system headers.'
