import sublime, sublime_plugin, html, time, datetime
from .Clara import *
from .EventListener import *

def claraPrint(message):
	t = datetime.datetime.fromtimestamp(time.time()).strftime('%X')
	print('Clara:{}: {}'.format(t, message))

# FIXME: Replace stubs
def verifyVersion(version):
	return 3126 <= int(version)

# FIXME: Replace stubs
def verifyPlatform(platform):
	return True

# FIXME: Add error messages
def plugin_loaded():
	if not verifyVersion(sublime.version()):
		sublime.error_message('Clara version mismatch.')
	if not verifyPlatform(sublime.platform()):
		sublime.error_message('Clara platform mismatch.')

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
		
		self.session = None

		# point -> list of tuples
		self.point2completions = {}

		# set of points (unordered) (python could use an ordered set datastructure)
		self.inFlightCompletionAtPoint = set()

		self.diagnosticPhantomSet = sublime.PhantomSet(self.view)
		self.newPhantoms = []

		if not hasCorrectExtension(self.view.file_name()):
			return

		claraPrint('Loading ' + self.view.file_name())
		options = SessionOptions()
		options.diagnosticCallback = self._diagnosticCallback
		options.logCallback = claraPrint
		options.codeCompleteCallback = self._completeCallback
		settings = sublime.load_settings('Clara.sublime-settings')
		options.filename = self.view.file_name()
		systemHeaders = self._loadHeaders('system_headers')
		builtinHeaders = self._loadHeaders('builtin_headers')
		options.systemHeaders = [""] if systemHeaders is None else systemHeaders
		options.builtinHeaders = '' if builtinHeaders is None else builtinHeaders
		# if options.filename.endswith('.hpp') or options.filename.endswith('.h'):
		# 	options.jsonCompileCommands == ""
		# else:
		# 	try:
		# 		project = self.view.window().project_data()
		# 		if project is None: raise Exception('No sublime-project found.')
		# 		cmakeSettings = project.get('cmake')
		# 		if cmakeSettings is None: raise Exception('No cmake settings found in sublime-project file.')
		# 		cmakeSettings = sublime.expand_variables(cmakeSettings, self.view.window().extract_variables())
		# 		rootFolder = cmakeSettings.get('root_folder')
		# 		buildFolder = cmakeSettings.get('build_folder')
		# 		options.jsonCompileCommands = "" if buildFolder is None else buildFolder
		# 	except Exception as e:
		# 		claraPrint(str(e))

		options.codeCompleteIncludeMacros = settings.get('include_macros', True)
		options.codeCompleteIncludeCodePatterns = settings.get('include_code_patterns', True)
		options.codeCompleteIncludeGlobals = settings.get('include_globals', True)
		options.codeCompleteIncludeBriefComments = settings.get('include_brief_comments', True)

		compdb = EventListener.getCompilationDatabase(self.view)
		if compdb:
			options.invocation, options.workingDirectory = compdb.get(self.view.file_name())
			claraPrint(options.invocation)
			claraPrint(options.workingDirectory)
		else:
			claraPrint('No compilation database found for "{}"'.format(self.view.file_name()))

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
			if point in self.inFlightCompletionAtPoint:
				claraPrint('Already completing at point {}, so wait.'.format(point))
				return None
			else:
				claraPrint('found no completions. Calling Session::codeCompleteAsync.')
				self.inFlightCompletionAtPoint.add(point)
				row, col = self.view.rowcol(point)
				unsavedBuffer = self.view.substr(sublime.Region(0, min(self.view.size(), point + 1)))
				row += 1 # clang rows are 1-based, sublime rows are 0-based
				col += 1 # clang columns are 1-based, sublime columns are 0-based
				self.session.codeCompleteAsync(unsavedBuffer, row, col, self._completeCallback)

	def on_post_save(self):
		claraPrint('on_post_save: {}'.format(self.session.filename))

	def _replaceSingleQuotesByBoldHtml(self, string):
		parts = string.split("'")
		result = ''
		inside = False
		for i, part in enumerate(parts):
			part = html.escape(part)
			result += part
			inside = not inside
			if i + 1 == len(parts): continue
			if inside: result += '<b>'
			else: result += '</b>'
		return result

	def _diagnosticCallback(self, filename, level, row, column, message):
		if filename != '' and filename != self.view.file_name():
			claraPrint("{}: {}:{}:{}: {}".format(level, filename, str(row), str(column), message))
			return
		divclass = None
		if level == 'begin':
			claraPrint('BEGIN DIAGNOSIS'.format(filename))
			# self.newPhantoms = []
			# assert isinstance(self.newPhantoms, list)
			# self.diagnosticPhantomSet.update(self.newPhantoms)
			return
		elif level == 'finish':
			claraPrint('END DIAGNOSIS'.format(filename))
			assert isinstance(self.newPhantoms, list)
			self.diagnosticPhantomSet.update(self.newPhantoms)
			self.newPhantoms = []
			return
		elif level == 'warning':
			claraPrint('{}, warning, {}, {}, {}'.format(filename, row, column, message))
			divclass = 'warning'
		elif level == 'error' or level == 'fatal':
			claraPrint('{}, error, {}, {}, {}'.format(filename, row, column, message))
			divclass = 'error'
		elif level == 'note':
			claraPrint('{}, note, {}, {}, {}'.format(filename, row, column, message))
			divclass = 'inserted'
		elif level == 'remark':
			claraPrint('{}, remark, {}, {}, {}'.format(filename, row, column, message))
			divclass = 'inserted'
		else:
			claraPrint('{}, unkown, {}, {}, {}'.format(filename, row, column, message))
			divclass = 'inserted'
		point = 0
		if row != -1 or column != -1:
			row -= 1
			column -= -1
			point = max(0, self.view.text_point(row, column) - 2)
		region = sublime.Region(point, point)
		message = self._replaceSingleQuotesByBoldHtml(message)
		message = '<body id="ClaraDiagnostic"><div class="{}"><i>{}</i></div></body>'.format(divclass, message)
		phantom = sublime.Phantom(region, message, sublime.LAYOUT_BELOW)
		self.newPhantoms.append(phantom)

	def _completeCallback(self, filename, row, col, completions):
		row -= 1 # Clang rows are 1-based, sublime rows are 0-based.
		col -= 1 # Clang columns are 1-based, sublime columns are 0-based.
		point = self.view.text_point(row, col)
		self.inFlightCompletionAtPoint.discard(point)
		if point in self.point2completions:
			claraPrint('point {} is already in my completion dictionary. :-( too late!'.format(point))
			return
		claraPrint('adding point {}'.format(point))
		if len(completions) == 0:
			completions = 'FOUND NOTHING'
		else:
			self.point2completions[point] = completions
			if self.view.is_auto_complete_visible():
				self.view.run_command('hide_auto_complete')
			self.view.run_command('auto_complete', {
				'disable_auto_insert': True,
				'api_completions_only': True,
				'next_competion_if_showing': False})
