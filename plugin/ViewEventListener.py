import sublime, sublime_plugin, tempfile
from .Clara import *
from .DiagnosticPrinter import *

def claraPrint(msg):
	print('Clara: ' + msg)

def plugin_loaded():
	claraPrint('Plugin loaded')

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
		self.phantoms = sublime.PhantomSet(self.view, 'Clara')
		self.session = None
		self.point2completions = {}
		claraPrint('Loading ' + self.view.file_name())
		options = SessionOptions()
		# options.diagnosticPrinter = self.diagnosticPrinter
		options.logCallback = claraPrint
		options.codeCompleteCallback = self._completeCallback
		settings = sublime.load_settings('Clara.sublime-settings')
		options.filename = self.view.file_name()
		systemHeaders = self._loadHeaders('system_headers')
		builtinHeaders = self._loadHeaders('builtin_headers')
		options.systemHeaders = [""] if systemHeaders is None else systemHeaders
		options.builtinHeaders = '' if builtinHeaders is None else builtinHeaders
		if options.filename.endswith('.hpp') or options.filename.endswith('.h'):
			options.jsonCompileCommands == ""
		else:
			try:
				project = self.view.window().project_data()
				if project is None: raise Exception('No sublime-project found.')
				cmakeSettings = project.get('cmake')
				if cmakeSettings is None: raise Exception('No cmake settings found in sublime-project file.')
				cmakeSettings = sublime.expand_variables(cmakeSettings, self.view.window().extract_variables())
				rootFolder = cmakeSettings.get('root_folder')
				buildFolder = cmakeSettings.get('build_folder')
				options.jsonCompileCommands = "" if buildFolder is None else buildFolder
			except Exception as e:
				claraPrint(str(e))

		options.codeCompleteIncludeMacros = settings.get('include_macros', True)
		options.codeCompleteIncludeCodePatterns = settings.get('include_code_patterns', True)
		options.codeCompleteIncludeGlobals = settings.get('include_globals', True)
		options.codeCompleteIncludeBriefComments = settings.get('include_brief_comments', True)
		options.diagnosticConsumer = DiagnosticPrinter(claraPrint)
		self.session = Session(options)

	def _loadHeaders(self, key):
		settings = sublime.load_settings('Clara.sublime-settings')
		headers = settings.get(key)
		if headers is not None:
			return headers
		elif sublime.ok_cancel_dialog('You do not yet have headers set up. Do you want to generate them now?'):
			sublime.run_command('generate_system_headers')
			return settings.get(key)
		else:
			sublime.error_message('Clara will not work without setting up headers!')
			return None

	def on_query_completions(self, prefix, locations):
		"""Find all possible completions."""
		point = locations[0]
		if (self.session is None or 
			len(locations) > 1 or 
			not self.view.match_selector(point, 'source.c++')):
			return None
		claraPrint('point = {}, prefix = {}'.format(point, prefix))
		completions = self.point2completions.pop(point, None)
		if completions:
			if completions == 'FOUND NOTHING':
				claraPrint('found completions via a callback, but no completions were returned.')
				return None
			else:
				claraPrint('found completions!')
				return (completions, sublime.INHIBIT_WORD_COMPLETIONS | sublime.INHIBIT_EXPLICIT_COMPLETIONS)
		else:
			claraPrint('found no completions. Calling Session::codeCompleteAsync.')
			row, col = self.view.rowcol(point)
			unsavedBuffer = self.view.substr(sublime.Region(0, min(self.view.size(), point + 1)))
			row += 1 # clang rows are 1-based, sublime rows are 0-based
			col += 1 # clang columns are 1-based, sublime columns are 0-based
			self.session.codeCompleteAsync(unsavedBuffer, row, col, self._completeCallback)

		# if self.point + len(prefix) == point:
		# 	if self.newCompletions:
		# 		claraPrint('delivering new completions')
		# 		completions = self.newCompletions
		# 		self.newCompletions = None
		# 		self.point = -1
		# 		return (completions, sublime.INHIBIT_WORD_COMPLETIONS | sublime.INHIBIT_EXPLICIT_COMPLETIONS)
		# 	else:
		# 		self.view.hide_popup()
		# 		self.view.show_popup('Still thinking...', sublime.COOPERATE_WITH_AUTO_COMPLETE, -1)
		# 		return None
		# else:
		# 	self.point = point
		# 	row, col = self.view.rowcol(point)
		# 	unsavedBuffer = self.view.substr(sublime.Region(0, min(self.view.size(), point + 1)))
		# 	row += 1 # clang rows are 1-based, sublime rows are 0-based
		# 	col += 1 # clang columns are 1-based, sublime columns are 0-based
		# 	claraPrint('code completing row {}, column {}, prefix {}'.format(row, col, prefix))
		# 	self.session.codeCompleteAsync(unsavedBuffer, row, col, self._completionCallback)
		# 	self.view.show_popup('Thinking...', sublime.COOPERATE_WITH_AUTO_COMPLETE, -1)
		# 	return None
			

		# else:
		# 	row, col = self.view.rowcol(point)
		# 	unsavedBuffer = self.view.substr(sublime.Region(0, min(self.view.size(), point + 1)))
		# 	row += 1 # clang rows are 1-based, sublime rows are 0-based
		# 	col += 1 # clang columns are 1-based, sublime columns are 0-based
		# 	completions = self.session.codeComplete(unsavedBuffer, row, col)
		# 	# print(completions)
		# 	return (completions, sublime.INHIBIT_WORD_COMPLETIONS | sublime.INHIBIT_EXPLICIT_COMPLETIONS)
		# elif self.noCompletionsFound:
		# 	self.newCompletions = None
		# 	self.noCompletionsFound = False
		# 	return None
		# elif self.point + len(prefix) == point and self.newCompletions is not None:
		# 	claraPrint('delivering NEW completions')
		# 	completions = self.newCompletions
		# 	self.newCompletions = None
		# 	self.noCompletionsFound = False
		# 	return (completions, sublime.INHIBIT_WORD_COMPLETIONS | sublime.INHIBIT_EXPLICIT_COMPLETIONS)
		# elif self.point + len(prefix) == point or self.point == point:
		# 	if self.newCompletions is not None:
		# 		self.noCompletionsFound = False
		# 		completions = self.newCompletions
		# 		self.newCompletions = None
		# 		return (completions, sublime.INHIBIT_WORD_COMPLETIONS | sublime.INHIBIT_EXPLICIT_COMPLETIONS)
		# else:
		# 	self.noCompletionsFound = False
		# 	self.point = point
		# 	row, col = self.view.rowcol(point)
		# 	unsavedBuffer = self.view.substr(sublime.Region(0, min(self.view.size(), point + 1)))
		# 	row += 1 # clang rows are 1-based, sublime rows are 0-based
		# 	col += 1 # clang columns are 1-based, sublime columns are 0-based
		# 	claraPrint('code completing row {}, column {}, prefix {}'.format(row, col, prefix))
		# 	self.session.codeCompleteAsync(unsavedBuffer, row, col, self._completionCallback)
		# 	self.view.show_popup('Thinking...', sublime.COOPERATE_WITH_AUTO_COMPLETE, -1)
		# 	return None

	# def on_modified_async(self):
		# sublime.set_timeout(self._runCompilation, 3000)

	# def _runCompilation(self):
	# 	unsavedBuffer = self.view.substr(sublime.Region(0, self.view.size()))
	# 	claraPrint('placeholder for running a compilation...')
		# self.session.compileAsync(unsavedBuffer, self._warningCallback, self._errorCallback, self._doneCompilationCallback)

	def _warningCallback(self, message):
		claraPrint(message)

	def _errorCallback(self, message):
		claraPrint(message)

	def _completeCallback(self, row, col, completions):
		row -= 1 # Clang rows are 1-based, sublime rows are 0-based.
		col -= 1 # Clang columns are 1-based, sublime columns are 0-based.
		point = self.view.text_point(row, col)
		if point in self.point2completions:
			claraPrint('point {} is already in my completion dictionary. :-( too late!'.format(point))
			return
		claraPrint('adding point {}'.format(point))
		if len(completions) == 0:
			completions = 'FOUND NOTHING'
		self.point2completions[point] = completions
		self.view.run_command('hide_auto_complete')
		self.view.run_command('auto_complete', {
			'disable_auto_insert': True,
			'api_completions_only': True,
			'next_competion_if_showing': False})

	# def _completionCallback(self, completions):
	# 	claraPrint('completions are ready, rerunning auto completion')
	# 	self.view.hide_popup()
	# 	assert self.newCompletions is None
	# 	self.newCompletions = completions
	# 	self.view.run_command('hide_auto_complete')
	# 	self.view.run_command('auto_complete', {
	# 		'disable_auto_insert': True,
	# 		'api_completions_only': True,
	# 		'next_competion_if_showing': False})
			
		


