import sublime, sublime_plugin, subprocess, os

SHELL_CMD = "clang++ -E -x c++ - -v < /dev/null 2>&1 | awk '/#include <...> search starts here/{f=1;next} /End of search list./{f=0} f'"

class GenerateSystemHeadersCommand(sublime_plugin.ApplicationCommand):
	"""Generates system header info."""

	def run(self):
		output = subprocess.check_output(SHELL_CMD, shell=True)
		output = output.decode('utf-8')
		headers = output.splitlines()
		headers = [os.path.abspath(x.strip()) for x in headers]
		settings = sublime.load_settings('Clara')
		settings.set('system_headers', headers)
		sublime.save_settings('Clara')
		sublime.message_dialog('System headers have been saved in user settings.')

	def description(self):
		return 'Generates system headers.'
