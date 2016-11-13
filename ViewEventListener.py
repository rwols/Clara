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
		self.newCompletions = None
		self.oldCompletions = None
		self.point = -1
		self.noCompletionsFound = False

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
		completions = [['\tPlease wait...', ' ']];
		return (completions, sublime.INHIBIT_WORD_COMPLETIONS | sublime.INHIBIT_EXPLICIT_COMPLETIONS)

	def on_query_completions(self, prefix, locations):
		"""Find all possible completions."""
		point = locations[0]
		if self.session is None or len(locations) > 1: return None
		elif not self.view.match_selector(point, 'source.c++'): return None
		elif self.noCompletionsFound:
			self.newCompletions = None
			self.noCompletionsFound = False
			return None
		elif self.point + len(prefix) == point and self.newCompletions is not None:
			claraPrint('delivering NEW completions')
			completions = self.newCompletions
			self.newCompletions = None
			self.noCompletionsFound = False
			return (completions, sublime.INHIBIT_WORD_COMPLETIONS | sublime.INHIBIT_EXPLICIT_COMPLETIONS)
		elif self.point + len(prefix) == point or self.point == point:
			if self.newCompletions is not None:
				self.noCompletionsFound = False
				completions = self.newCompletions
				self.newCompletions = None
				return (completions, sublime.INHIBIT_WORD_COMPLETIONS | sublime.INHIBIT_EXPLICIT_COMPLETIONS)
		else:
			self.noCompletionsFound = False
			self.point = point
			row, col = self.view.rowcol(point)
			unsavedBuffer = self.view.substr(sublime.Region(0, min(self.view.size(), point + 1)))
			row += 1 # clang rows are 1-based, sublime rows are 0-based
			col += 1 # clang columns are 1-based, sublime columns are 0-based
			claraPrint('code completing row {}, column {}, prefix {}'.format(row, col, prefix))
			self.session.codeCompleteAsync(unsavedBuffer, row, col, self._completionCallback)
			return self._tempCompletionMessage()

	def _completionCallback(self, completions):
		claraPrint('completions are ready, rerunning auto completion')
		# claraPrint('note: completions are {}'.format(completions))
		if len(completions) == 0:
			claraPrint('no completions were found!')
			self.noCompletionsFound = True
			self.newCompletions = None
		else:
			self.noCompletionsFound = False
			self.newCompletions = completions
		self.view.run_command('hide_auto_complete')
		# self.view.run_command('auto_complete')
		self.view.run_command('auto_complete', {
			'disable_auto_insert': True,
			'api_completions_only': False,
			'next_competion_if_showing': True})
			
		


