import sublime        # various functionality
import sublime_plugin # for ViewEventListener
import os             # for path handling
import imp            # for loading the dynamic library

Clara = None

def plugin_loaded():
	print('plugin_loaded called')
	pwd = os.path.dirname(os.path.abspath(__file__))
	claraPath = os.path.join(pwd, 'build/lib/Clara.so')
	global Clara
	Clara = imp.load_dynamic('Clara', claraPath)

class ClaraViewListener(sublime_plugin.ViewEventListener):

	@classmethod
	def is_applicable(settings):
		return 'C++' in setting.get('syntax')

	@classmethod
	def applies_to_primary_view_only():
		return False

	def __init__(self):
		super(ClaraViewListener, self).__init__()
		self.session = Clara.Session()

	def pointIsRevelant(self, point):
		return self.view.match_selector(point, 'source.c++')

	def sublimeRowCol2ClangRowCol(self, row, col):
		return row + 1, col + 1

	def getBuffer(self):
		return self.view.substr(sublime.Region(0, view.size()))

	def getFilenameAndBufferRowCol(self, point):
		buffer = self.getBuffer(view)
		row, col = self.view.rowcol(point)
		return self.view.file_name(), buffer, row, col

	def on_query_completions(self, prefix, locations):
		completions = [] # Start with an empty list.
		for point in locations:
			if not self.pointIsRevelant(point): continue
			line = view.substr(self.view.line(point))
			numTabs = line.count('\t')
			tabSize = self.view.settings().get('tab_size')
			filename, buffer, row, column = self.getFilenameAndBufferRowCol(view, point)
			print('sublime ({0},{1})'.format(row, column))
			row, column = self.sublimeRowCol2ClangRowCol(row, column)
			print('clang ({0},{1})'.format(row, column))
			completion = self.session.codeComplete(filename, buffer, row, col)
			completions.extend(completion)
		print(completions) # This is only for debug purposes.
		return completions