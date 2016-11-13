import sublime, sublime_plugin
from .cpp import *

def claraPrint(msg):
	print('Clara: ' + msg)

class ViewEventListener(sublime_plugin.ViewEventListener):
	"""Watches views."""

	@classmethod
	def is_applicable(cls, settings):
		"""Returns True if the syntax is C++, otherwise false."""
		return 'C++' in settings.get('syntax')

	@classmethod
	def applies_to_primary_view_only(cls):
		"""This ViewEventListener applies to all views, primary or not."""
		return False

	def __init__(self, view):
		"""Initializes this ViewEventListener."""
		super(ViewEventListener, self).__init__(view)
		# self.consumer = DiagnosticConsumer()
		self.session = None
		self.completions = None
		self.point = -1

	def on_activated_async(self):
		"""Checks wether we already have an active session object.
		If not, construct one."""
		if self.session is not None: return
		claraPrint('Loading Clara session, please wait.')
		project = self.view.window().project_data()
		try:
			if project is None: raise Exception('No sublime-project found.')
			claraSettings = project.get('clara')
			if claraSettings is None: raise Exception('No settings found in sublime-project file.')
			cmakeFile = claraSettings.get('cmake_file')
			buildFolder = claraSettings.get('build_folder')
			self.session = Session(self.view.file_name(), buildFolder)
			claraPrint('Loaded ' + self.view.file_name() + ' with compile commands.')
		except Exception as e:
			claraPrint(str(e))
			self.session = Session(self.view.file_name())
		self.session.reporter = claraPrint
		self.view.erase_status('Clara')

	def _tempCompletionMessage(self):
		completions = [['\tThinking...', '\t']];
		return (completions, sublime.INHIBIT_WORD_COMPLETIONS | sublime.INHIBIT_EXPLICIT_COMPLETIONS)

	def on_query_completions(self, prefix, locations):
		"""Find all possible completions."""
		if self.session is None or len(locations) > 1: return None
		point = locations[0]
		if not self.view.match_selector(point, 'source.c++'): return None

		# if self.completions is None:
		# 	if self.point + len(prefix) == point:
		# 		return self._tempCompletionMessage()

		# if self.point == point:
		# 	completions = self.completions
		# 	self.completions = None
		# 	return (completions, sublime.INHIBIT_WORD_COMPLETIONS | sublime.INHIBIT_EXPLICIT_COMPLETIONS)
		
		# if self.point + len(prefix) == point:
		# 	# completing at the same location basically
		# 	if self.completions is not None:
		# 		completions = self.completions
		# 		self.completions = None
		# 		return (completions, sublime.INHIBIT_WORD_COMPLETIONS | sublime.INHIBIT_EXPLICIT_COMPLETIONS)
		# 	else:
		# 		return self._tempCompletionMessage()

		if self.completions is not None and self.point == point:
			completions = self.completions
			self.completions = None
			return (completions, sublime.INHIBIT_WORD_COMPLETIONS | sublime.INHIBIT_EXPLICIT_COMPLETIONS)
		self.point = point
		row, col = self.view.rowcol(point)
		unsavedBuffer = self.view.substr(sublime.Region(0, point))
		row += 1 # clang rows are 1-based, sublime rows are 0-based
		col += 1 # clang columns are 1-based, sublime columns are 0-based
		claraPrint('code completing point {}, prefix {}'.format(point, prefix))
		self.session.codeCompleteAsync(unsavedBuffer, row, col, self._completionCallback)
		return None
		# return self._tempCompletionMessage()

	def _completionCallback(self, completions):
		# claraPrint('running callback method')
		self.completions = completions
		self.view.run_command('hide_auto_complete')
		self.view.run_command('auto_complete', {
	        'disable_auto_insert': True,
	        'api_completions_only': False,
	        'next_competion_if_showing': True})


