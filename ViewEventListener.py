import sublime
import sublime_plugin
import os
import imp

# This ugly crap is here because I can't seem to just do "import Clara"
Clara = imp.load_dynamic('Clara', os.path.join(os.path.dirname(os.path.abspath(__file__)), 'Clara.so'))

class ViewEventListener(sublime_plugin.ViewEventListener):
	"""Watches views."""

	def __init__(self, view):
		"""Initializes this ViewEventListener."""
		super(ViewEventListener, self).__init__(view)
		self.session = Clara.Session()

	@classmethod
	def is_applicable(cls, settings):
		"""Returns True if the syntax is C++, otherwise false."""
		return 'C++' in settings.get('syntax')

	@classmethod
	def applies_to_primary_view_only(cls):
		"""This ViewEventListener applies to all views, primary or not."""
		return False

	def on_query_completions(self, prefix, locations):
		"""Find all possible completions."""
		if len(locations) > 1: return None
		point = locations[0]
		if not self.view.match_selector(point, 'source.c++'): return None
		line = self.view.substr(self.view.line(point))
		numTabs = line.count('\t')
		tabSize = self.view.settings().get('tab_size')
		unsavedBuffer = self.view.substr(sublime.Region(0, self.view.size()))
		row, col = self.view.rowcol(point)
		filename = self.view.file_name()
		row += 1                 # clang rows are 1-based, sublime rows are 0-based
		col += 1                 # clang columns are 1-based, sublime columns are 0-based
		col -= numTabs * tabSize # sublime counts tabs as spaces, clang doesn't
		col += numTabs           # tabs count as one character
		completions = self.session.codeComplete(filename, unsavedBuffer, row, col)
		print(completions)
		return completions

		