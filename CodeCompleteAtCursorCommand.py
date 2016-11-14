import sublime, sublime_plugin

class CodeCompleteAtCursorCommand(sublime_plugin.TextCommand):

	def run(self, edit):
		self.view.run_command('hide_auto_complete')
		self.view.run_command('auto_complete')