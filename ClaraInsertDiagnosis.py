import sublime, sublime_plugin

CLANG = "clang++"

class ClaraInsertDiagnosisCommand(sublime_plugin.TextCommand):

	def run(self, edit):
		self.view.insert(edit, 0, 'hello, world!')