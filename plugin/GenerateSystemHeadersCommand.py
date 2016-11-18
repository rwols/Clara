import sublime, sublime_plugin, subprocess, os

class GenerateSystemHeadersCommand(sublime_plugin.ApplicationCommand):
	"""Generates system header info."""

	def run(self):
		
		SHELL_CMD1 = "clang++ -E -x c++ - -v < /dev/null 2>&1 | awk '/#include <...> search starts here/{f=1;next} /End of search list./{f=0} f'"
		SHELL_CMD2 = "clang++ -E -x c++ - -v < /dev/null 2>&1 | awk 'BEGIN{FS=\"-resource-dir\"} /-resource-dir/ {print $2}' | awk '{print $1;}'"
		USER_MSG = 'System headers have been saved in user settings. Do you want to view them now?'
		SETTINGS = 'Clara.sublime-settings'

		output = subprocess.check_output(SHELL_CMD1, shell=True)
		output = output.decode('utf-8')
		headers = output.splitlines()
		for i, header in enumerate(headers):
			header = header.strip()
			if header.endswith(' (framework directory)'):
				header = header[:-len(' (framework directory)')]
			headers[i] = os.path.abspath(header)
		output = subprocess.check_output(SHELL_CMD2, shell=True)
		builtinHeaders = os.path.abspath(output.decode('utf-8').strip())
		settings = sublime.load_settings(SETTINGS)
		settings.set('system_headers', headers)
		settings.set('builtin_headers', builtinHeaders)
		sublime.save_settings(SETTINGS)
		if sublime.ok_cancel_dialog(USER_MSG):
			sublime.run_command('open_clara_settings')

	def description(self):
		return 'Generates system headers.'
