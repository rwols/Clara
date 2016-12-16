import sublime, sublime_plugin, html, threading
from .Clara import *
from .EventListener import *

# _hasLoadedHeadersAtleastOnce = False

# # FIXME: Replace stubs
# def verifyVersion():
# 	return 3126 <= int(sublime.version())

# # FIXME: Replace stubs
# def verifyPlatform():
# 	platform = sublime.platform()
# 	return True

# # FIXME: Add error messages
# def plugin_loaded():
# 	claraPrint('ViewEventListener loaded.')
# 	if not verifyVersion():
# 		sublime.error_message('Clara version mismatch.')
# 	if not verifyPlatform():
# 		sublime.error_message('Clara platform mismatch.')

# class ViewEventListener(sublime_plugin.ViewEventListener):
# 	"""Watches views."""

# 	@classmethod
# 	def is_applicable(cls, settings):
# 		"""Returns True if the syntax is C++, otherwise false."""
# 		return 'C++' in settings.get('syntax')

# 	@classmethod
# 	def applies_to_primary_view_only(cls):
# 		"""This ViewEventListener applies to all views, primary or not."""
# 		return False

# 	def __init__(self, view):
# 		"""Initializes this ViewEventListener."""
# 		super(ViewEventListener, self).__init__(view)
# 		self.session = None
# 		self.sessionIsLoading = False
# 		# point -> list of tuples
# 		self.point2completions = {}
# 		# set of points (unordered) (python could use an ordered set datastructure)
# 		self.inFlightCompletionAtPoint = set()
# 		self.diagnosticPhantomSet = sublime.PhantomSet(self.view)
# 		self.newPhantoms = []
# 		self.numTimesCompletionsDelivered = 0
# 		self.isReparsing = False

# 	def _initSession(self):
# 		if self.sessionIsLoading:
# 			# Already loading a session in a thread somewhere.
# 			return

# 		# This boolean variable will stay true forever. It's a useful check
# 		# to see if we *tried* to load a session (but perhaps failed). If we
# 		# failed once, we're just not gonna try again anymore.
# 		self.sessionIsLoading = True

# 		if not hasCorrectExtension(self.view.file_name()):
# 			# The file doesn't have the correct extension, so forget about it.
# 			return
# 		compdb = EventListener.getCompilationDatabase(self.view)
# 		if not compdb:
# 			# Not even going to try.
# 			return

# 		# Start building the SessionOptions object, to build a Session object.
# 		options = SessionOptions()
# 		options.invocation, options.workingDirectory = compdb.get(self.view.file_name())
# 		if not options.invocation:
# 			# This file is not in the compilation database, so forget about it.
# 			claraPrint('The file "{}" is not in the compilation database, so we are not doing code-completion.'
# 				.format(self.view.file_name()))
# 			return

# 		# Load in the system headers and builtin headers.
# 		systemHeaders = self._loadHeaders('system_headers')
# 		builtinHeaders = self._loadHeaders('builtin_headers')
# 		frameworks = self._loadHeaders('system_frameworks')
# 		options.systemHeaders = [""] if systemHeaders is None else systemHeaders
# 		options.builtinHeaders = '' if builtinHeaders is None else builtinHeaders
# 		options.frameworks = '' if frameworks is None else frameworks

# 		# At this point we can't really fail, so set the view's status to
# 		# something informative to let the user know that we are parsing.
# 		self.view.set_status('Clara', 'Parsing file for auto-completion, this can take a while!')
# 		claraPrint('Loading "{}" with working directory "{}" and compiler invocation {}'
# 			.format(self.view.file_name(), options.workingDirectory, str(options.invocation)))

# 		options.diagnosticCallback = self._diagnosticCallback
# 		options.logCallback = claraPrint
# 		options.codeCompleteCallback = self._completeCallback
# 		settings = sublime.load_settings('Clara.sublime-settings')
# 		options.filename = self.view.file_name()

# 		options.codeCompleteIncludeMacros = settings.get('include_macros', True)
# 		options.codeCompleteIncludeCodePatterns = settings.get('include_code_patterns', True)
# 		options.codeCompleteIncludeGlobals = settings.get('include_globals', True)
# 		options.codeCompleteIncludeBriefComments = settings.get('include_brief_comments', True)

# 		self.session = Session(options)
# 		claraPrint('Loaded "{}"'.format(self.view.file_name()))
# 		self.view.erase_status('Clara')

# 	def _loadHeaders(self, key):
# 		settings = sublime.load_settings('Clara.sublime-settings')
# 		headers = settings.get(key)
# 		global _hasLoadedHeadersAtleastOnce
# 		if headers is not None:
# 			return headers
# 		elif not _hasLoadedHeadersAtleastOnce:
# 			_hasLoadedHeadersAtleastOnce = True
# 			if sublime.ok_cancel_dialog('You do not yet have headers set up. Do you want to generate them now?'):
# 				sublime.run_command('generate_system_headers')
# 				return settings.get(key)
# 			else:
# 				sublime.error_message('Clara will not work properly without setting up headers!')
# 				return None
# 		else:
# 			return None

# 	def _reparse(self):
# 		claraPrint('Reparsing {}'.format(self.view.file_name()))
# 		self.isReparsing = True
# 		self.session.reparse(self.view.id(), self._reparseCallback)
		

# 	def _reparseCallback(self, viewID, success):
# 		self.isReparsing = False
# 		if success:
# 			claraPrint('Reparsed for view {}'.format(viewID))
# 		else:
# 			claraPrint('Error occured while reparsing for view {}!'.format(viewID))

# 	def on_activated(self):
# 		# We're not using on_activated_async because it seems
# 		# to restrict itself to a single background thread.
# 		# Some implementation files may take a long time to parse,
# 		# and if the user has already clicked a new view the on_activated_async
# 		# method of that view will wait for the previous one to finish. We
# 		# don't need that synchronization, so we spawn a thread for each new
# 		# view.
# 		if self.session or self.sessionIsLoading:
# 			return
# 		else:
# 			threading.Thread(target=self._initSession).start()

# 	#def on_activated_async(self):
# 	#	self._initSession()

# 	def on_query_completions(self, prefix, locations):
# 		"""Find all possible completions."""
# 		if not self.session or self.isReparsing:
# 			return
# 		point = locations[0]
# 		if len(locations) > 1:
# 			return
# 		if not self.view.match_selector(point, 'source.c++'):
# 			return
# 		claraPrint('point = {}, prefix = {}'.format(point, prefix))
# 		completions = self.point2completions.pop(point, None)
# 		if completions:
# 			if completions == 'FOUND NOTHING':
# 				claraPrint('found completions via a callback, but no completions were returned.')
# 				return None
# 			else:
# 				claraPrint('found completions!')
# 				self.numTimesCompletionsDelivered += 1
# 				if self.numTimesCompletionsDelivered > 4:
# 					self.numTimesCompletionsDelivered = 0
# 					threading.Thread(target=self._reparse).start()
# 				return (completions, sublime.INHIBIT_WORD_COMPLETIONS | sublime.INHIBIT_EXPLICIT_COMPLETIONS)
# 		else:
# 			if point in self.inFlightCompletionAtPoint:
# 				claraPrint('Already completing at point {}, so wait.'.format(point))
# 				return None
# 			else:
# 				claraPrint('found no completions. Calling Session::codeCompleteAsync.')
# 				self.inFlightCompletionAtPoint.add(point)
# 				row, col = self.view.rowcol(point)
# 				unsavedBuffer = self.view.substr(sublime.Region(0, self.view.size()))
# 				row += 1 # clang rows are 1-based, sublime rows are 0-based
# 				col += 1 # clang columns are 1-based, sublime columns are 0-based
# 				self.session.codeCompleteAsync(unsavedBuffer, row, col, self._completeCallback)

# 	def _replaceSingleQuotesByBoldHtml(self, string):
# 		parts = string.split("'")
# 		result = ''
# 		inside = False
# 		for i, part in enumerate(parts):
# 			part = html.escape(part)
# 			result += part
# 			inside = not inside
# 			if i + 1 == len(parts): continue
# 			if inside: result += '<b>'
# 			else: result += '</b>'
# 		return result

# 	def _diagnosticCallback(self, filename, level, row, column, message):
# 		if filename != '' and filename != self.view.file_name():
# 			claraPrint("{}: {}:{}:{}: {}".format(level, filename, str(row), str(column), message))
# 			return
# 		divclass = None
# 		if level == 'begin':
# 			claraPrint('BEGIN DIAGNOSIS'.format(filename))
# 			# self.newPhantoms = []
# 			# assert isinstance(self.newPhantoms, list)
# 			# self.diagnosticPhantomSet.update(self.newPhantoms)
# 			return
# 		elif level == 'finish':
# 			claraPrint('END DIAGNOSIS'.format(filename))
# 			self.newPhantoms = []
# 			self.diagnosticPhantomSet.update(self.newPhantoms)
# 			return
# 		elif level == 'warning':
# 			claraPrint('{}, warning, {}, {}, {}'.format(filename, row, column, message))
# 			divclass = 'warning'
# 		elif level == 'error' or level == 'fatal':
# 			claraPrint('{}, error, {}, {}, {}'.format(filename, row, column, message))
# 			divclass = 'error'
# 		elif level == 'note':
# 			claraPrint('{}, note, {}, {}, {}'.format(filename, row, column, message))
# 			divclass = 'inserted'
# 		elif level == 'remark':
# 			claraPrint('{}, remark, {}, {}, {}'.format(filename, row, column, message))
# 			divclass = 'inserted'
# 		else:
# 			claraPrint('{}, unkown, {}, {}, {}'.format(filename, row, column, message))
# 			divclass = 'inserted'
# 		point = 0
# 		if row != -1 or column != -1:
# 			row -= 1
# 			column -= -1
# 			# The reason is not clear, but Clang somehow puts error too much to the right
# 			# (two character points to be precise), so we subtract that here again to
# 			# make the phantoms line up at the exact error.
# 			point = max(0, self.view.text_point(row, column) - 2)
# 		region = sublime.Region(point, point)
# 		message = self._replaceSingleQuotesByBoldHtml(message)
# 		message = '<body id="ClaraDiagnostic"><div class="{}"><i>{}</i></div></body>'.format(divclass, message)
# 		phantom = sublime.Phantom(region, message, sublime.LAYOUT_BELOW)
# 		self.newPhantoms.append(phantom)
# 		self.diagnosticPhantomSet.update(self.newPhantoms)

# 	def _completeCallback(self, filename, row, col, completions):
# 		row -= 1 # Clang rows are 1-based, sublime rows are 0-based.
# 		col -= 1 # Clang columns are 1-based, sublime columns are 0-based.
# 		point = self.view.text_point(row, col)
# 		self.inFlightCompletionAtPoint.discard(point)
# 		if point in self.point2completions:
# 			claraPrint('point {} is already in my completion dictionary. :-( too late!'.format(point))
# 			return
# 		claraPrint('adding point {}'.format(point))
# 		if len(completions) == 0:
# 			completions = 'FOUND NOTHING'
# 		else:
# 			self.point2completions[point] = completions
# 			if self.view.is_auto_complete_visible():
# 				self.view.run_command('hide_auto_complete')
# 			self.view.run_command('auto_complete', {
# 				'disable_auto_insert': True,
# 				'api_completions_only': True,
# 				'next_competion_if_showing': False})
