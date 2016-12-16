import sublime, sublime_plugin, os, time, datetime, threading
from .Clara import *

_printLock = threading.Lock()
_hasLoadedHeadersAtleastOnce = False

# FIXME: Replace stubs
def verify_version():
	return 3126 <= int(sublime.version())

# FIXME: Replace stubs
def verify_platform():
	platform = sublime.platform()
	return True

# FIXME: Add error messages
def plugin_loaded():
	if not verify_version():
		sublime.error_message('Clara version mismatch.')
	if not verify_platform():
		sublime.error_message('Clara platform mismatch.')
	clara_print('EventListener loaded.')

def clara_print(message):
	with _printLock as lock:
		t = datetime.datetime.fromtimestamp(time.time()).strftime('%X')
		print('Clara:{}: {}'.format(t, message))

def has_correct_extension(filename):
	return os.path.splitext(filename)[1] in ['.cpp', '.cc', '.c', '.cxx', '.objc']

def replace_single_quotes_by_tag(message, tag):
	parts = message.split("'")
	result = ''
	inside = False
	for i, part in enumerate(parts):
		part = html.escape(part)
		result += part
		inside = not inside
		if i + 1 == len(parts): continue
		if inside: result += '<{}>'.format(tag)
		else: result += '</{}>'.format(tag)
	return result

class ViewData(object):
	"""ViewData"""
	def __init__(self, view):
		super(ViewData, self).__init__()
		self.view = view
		self.phantomSet = sublime.PhantomSet(view)
		self.phantoms = []

class FileBufferData(object):
	"""FileBufferData"""
	def __init__(self, initial_view):
		super(FileBufferData, self).__init__()
		self.file_name = initial_view.file_name()
		clara_print('Created new FileBufferData for file "{}"'.format(self.file_name))
		assert has_correct_extension(self.file_name)
		self.session = None
		self.session_is_loading = True
		self.is_reparsing = False
		self.views = {}
		self.views[initial_view.id()] = ViewData(initial_view)
		self.initial_view = initial_view
		self.point_to_completions = {}
		self.inflight_completions = set()
		threading.Thread(target=self._initialize_session).start()

	def __del__(self):
		clara_print('Closed: {}'.format(self.file_name))

	def add_view(self, new_view):
		if not self.has_view(new_view):
			self.views[view.id()] = ViewData(new_view)

	def has_view(self, view):
		return view.id() in self.views

	def remove_view(self, view):
		self.views.pop(view.id(), None)

	def set_status(self, message):
		for view_id, view_data in self.views.items():
			view_data.view.set_status('Clara', message)

	def erase_status(self):
		for view_id, view_data in self.views.items():
			view_data.view.erase_status('Clara')

	def on_post_save(self):
		if self.session and not self.session_is_loading and not self.is_reparsing:
			threading.Thread(target=self._reparse).start()

	def on_query_completions(self, view, prefix, locations):
		assert view.id() in self.views
		if not self.session or self.session_is_loading or self.is_reparsing or len(locations) > 1:
			return None
		point = locations[0]
		# if (not view.match_selector(point, 'source.c++') or
		# 	('-' in prefix and not '->' in prefix) or
		# 	':' in prefix):
		# 	return None
		if not view.match_selector(point, 'source.c++'):
			return None
		clara_print('popping point {}'.format(point))
		completions = self.point_to_completions.pop(point, None)
		if completions:
			if completions == 'FOUND NOTHING':
				clara_print('Did auto-completion, but found nothing.')
				return None
			else:
				clara_print('Delivering completions')
				return (completions, sublime.INHIBIT_WORD_COMPLETIONS | sublime.INHIBIT_EXPLICIT_COMPLETIONS)
		else:
			if point in self.inflight_completions:
				clara_print('Already completing at point {}'.format(point))
				return None
			else:
				clara_print('Starting new auto-completion run at point {}'.format(point))
				self.inflight_completions.add(point)
				row, col = view.rowcol(point)
				unsaved_buffer = view.substr(sublime.Region(0, view.size()))
				row += 1 # clang rows are 1-based, sublime rows are 0-based
				col += 1 # clang columns are 1-based, sublime columns are 0-based
				self.session.codeCompleteAsync(view.id(), unsaved_buffer, row, col, self._completion_callback)

	# def on_query_completions(self, prefix, locations):
	# 	"""Find all possible completions."""
	# 	if not self.session or self.isReparsing:
	# 		return
	# 	point = locations[0]
	# 	if len(locations) > 1:
	# 		return
	# 	if not self.view.match_selector(point, 'source.c++'):
	# 		return
	# 	clara_print('point = {}, prefix = {}'.format(point, prefix))
	# 	completions = self.point2completions.pop(point, None)
	# 	if completions:
	# 		if completions == 'FOUND NOTHING':
	# 			clara_print('found completions via a callback, but no completions were returned.')
	# 			return None
	# 		else:
	# 			clara_print('found completions!')
	# 			self.numTimesCompletionsDelivered += 1
	# 			if self.numTimesCompletionsDelivered > 4:
	# 				self.numTimesCompletionsDelivered = 0
	# 				threading.Thread(target=self._reparse).start()
	# 			return (completions, sublime.INHIBIT_WORD_COMPLETIONS | sublime.INHIBIT_EXPLICIT_COMPLETIONS)
	# 	else:
	# 		if point in self.inFlightCompletionAtPoint:
	# 			clara_print('Already completing at point {}, so wait.'.format(point))
	# 			return None
	# 		else:
	# 			clara_print('found no completions. Calling Session::codeCompleteAsync.')
	# 			self.inFlightCompletionAtPoint.add(point)
	# 			row, col = self.view.rowcol(point)
	# 			unsavedBuffer = self.view.substr(sublime.Region(0, self.view.size()))
	# 			row += 1 # clang rows are 1-based, sublime rows are 0-based
	# 			col += 1 # clang columns are 1-based, sublime columns are 0-based
	# 			self.session.codeCompleteAsync(unsavedBuffer, row, col, self._completion_callback)

	def _diagnostic_callback(self, filename, severity, row, column, message):
		active_view = sublime.active_window().active_view()
		if filename != '' and filename != self.file_name:
			clara_print("{}: {}:{}:{}: {}".format(severity, filename, str(row), str(column), message))
			return
		view_data = self.views[active_view.id()]
		div_class = None
		if severity == 'begin':
			clara_print('BEGIN DIAGNOSIS'.format(filename))
			# self.newPhantoms = []
			# assert isinstance(self.newPhantoms, list)
			# self.diagnosticPhantomSet.update(self.newPhantoms)
			return
		elif severity == 'finish':
			clara_print('END DIAGNOSIS'.format(filename))
			view_data.newPhantoms = []
			view_data.diagnosticPhantomSet.update(view_data.newPhantoms)
			return
		elif severity == 'warning':
			clara_print('{}, warning, {}, {}, {}'.format(filename, row, column, message))
			div_class = 'warning'
		elif severity == 'error' or severity == 'fatal':
			clara_print('{}, error, {}, {}, {}'.format(filename, row, column, message))
			div_class = 'error'
		elif severity == 'note':
			clara_print('{}, note, {}, {}, {}'.format(filename, row, column, message))
			div_class = 'inserted'
		elif severity == 'remark':
			clara_print('{}, remark, {}, {}, {}'.format(filename, row, column, message))
			div_class = 'inserted'
		else:
			clara_print('{}, unkown, {}, {}, {}'.format(filename, row, column, message))
			div_class = 'inserted'
		point = 0
		if row != -1 or column != -1:
			row -= 1
			column -= -1
			# The reason is not clear, but Clang somehow puts error too much to the right
			# (two character points to be precise), so we subtract that here again to
			# make the phantoms line up at the exact error.
			point = max(0, view_data.view.text_point(row, column) - 2)
		region = sublime.Region(point, point)
		message = replace_single_quotes_by_tag(message, 'b')
		message = '<body id="Clara"><div id="diagnostic" class="{}">{}</div></body>'.format(div_class, message)
		phantom = sublime.Phantom(region, message, sublime.LAYOUT_BELOW)
		view_data.newPhantoms.append(phantom)
		view_data.diagnosticPhantomSet.update(view_data.newPhantoms)

	def _completion_callback(self, view_id, row, col, completions):
		row -= 1 # Clang rows are 1-based, sublime rows are 0-based.
		col -= 1 # Clang columns are 1-based, sublime columns are 0-based.
		view = self.views[view_id].view
		point = view.text_point(row, col)
		self.inflight_completions.discard(point)
		if sublime.active_window().active_view() != view:
			# Too late, user is not interested anymore.
			return
		if point in self.point_to_completions:
			clara_print('point {} is already in my completion dictionary. :-( too late!'.format(point))
			return
		clara_print('adding point {}'.format(point))
		if len(completions) == 0:
			completions = 'FOUND NOTHING'
		else:
			self.point_to_completions[point] = completions
			if view.is_auto_complete_visible():
				view.run_command('hide_auto_complete')
			view.run_command('auto_complete', {
				'disable_auto_insert': True,
				'api_completions_only': True,
				'next_competion_if_showing': False})

	def _load_headers(self, key):
		settings = sublime.load_settings('Clara.sublime-settings')
		headers = settings.get(key)
		global _hasLoadedHeadersAtleastOnce
		if headers is not None:
			return headers
		elif not _hasLoadedHeadersAtleastOnce:
			_hasLoadedHeadersAtleastOnce = True
			if sublime.ok_cancel_dialog('You do not yet have headers set up. Do you want to generate them now?'):
				sublime.run_command('generate_system_headers')
				return settings.get(key)
			else:
				sublime.error_message('Clara will not work properly without setting up headers!')
				return None
		else:
			return None

	def _initialize_session(self):
		if not has_correct_extension(self.file_name):
			# The file doesn't have the correct extension, so forget about it.
			return
		compdb = EventListener.getCompilationDatabase(self.initial_view)
		if not compdb:
			# Not even going to try.
			return

		# Start building the SessionOptions object, to build a Session object.
		options = SessionOptions()
		options.invocation, options.workingDirectory = compdb.get(self.file_name)
		if not options.invocation:
			# This file is not in the compilation database, so forget about it.
			clara_print('The file "{}" is not in the compilation database, so we are not doing code-completion.'
				.format(self.file_name))
			return

		# Load in the system headers and builtin headers.
		system_headers = self._load_headers('system_headers')
		frameworks = self._load_headers('system_frameworks')
		options.systemHeaders = [""] if system_headers is None else system_headers
		# options.builtinHeaders = '' if builtinHeaders is None else builtinHeaders
		options.frameworks = '' if frameworks is None else frameworks

		# At this point we can't really fail, so set the view's status to
		# something informative to let the user know that we are parsing.
		self.set_status('Parsing file for auto-completion, this can take a while!')
		clara_print('Loading "{}" with working directory "{}" and compiler invocation {}'
			.format(self.file_name, options.workingDirectory, str(options.invocation)))

		options.diagnosticCallback = self._diagnostic_callback
		options.logCallback = clara_print
		options.codeCompleteCallback = self._completion_callback
		settings = sublime.load_settings('Clara.sublime-settings')
		options.filename = self.file_name

		options.codeCompleteIncludeMacros = settings.get('include_macros', True)
		options.codeCompleteIncludeCodePatterns = settings.get('include_code_patterns', True)
		options.codeCompleteIncludeGlobals = settings.get('include_globals', True)
		options.codeCompleteIncludeBriefComments = settings.get('include_brief_comments', True)

		self.session = Session(options)
		self.session_is_loading = False # Success
		clara_print('Loaded "{}"'.format(self.file_name))
		self.erase_status()

	def _reparse(self):
		clara_print('Reparsing {}'.format(self.file_name))
		self.is_reparsing = True
		self.session.reparse(0, self._reparseCallback)
		

	def _reparseCallback(self, view_id, success):
		self.is_reparsing = False
		if success:
			clara_print('Reparsed "{}"'.format(self.file_name))
		else:
			clara_print('Error occured while reparsing "{}"'.format(self.file_name))
			self.session = None
			self.session_is_loading = True


class EventListener(sublime_plugin.EventListener):

	compilationDatabases = {}
	activeWindows = set()

	@classmethod
	def getCompilationDatabase(cls, view):
		cls._ensureExistenceOfCompilationDatabase(view)
		try:
			database = cls.compilationDatabases[view.window().id()]
			return database
		except KeyError as keyError:
			clara_print('No database found.')
			return None
		except AttributeError as attrError:
			clara_print('View has no window anymore.')
			return None

	def __init__(self):
		super(EventListener, self).__init__()
		self.file_buffers = {}

	def on_new(self, view):
		clara_print('on_new: {}, {}'.format(view.id(), view.file_name()))
		if EventListener._ensureExistenceOfCompilationDatabase(view):
			if not self._ensure_view_is_loaded(view):
				clara_print('Did not load {}, {}'.format(view.id(), view.file_name()))

	def on_load(self, view):
		clara_print('on_load: {}, {}'.format(view.id(), view.file_name()))
		self.on_new(view)

	def on_clone(self, view):
		clara_print('on_clone: {}, {}'.format(view.id(), view.file_name()))
		self.on_new(view)

	def on_activated(self, view):
		clara_print('on_activated: {}, {}'.format(view.id(), view.file_name()))
		self.on_new(view)

	def on_close(self, view):
		clara_print('on_close: {}, {}'.format(view.id(), view.file_name()))
		self._remove_view_from_file_buffers(view)

	def on_post_save(self, view):
		file_buffer_data = self._get_file_buffer_data_for_view(view)
		if file_buffer_data:
			file_buffer_data.on_post_save()

	def on_query_completions(self, view, prefix, locations):
		file_buffer_data = self._get_file_buffer_data_for_view(view)
		if file_buffer_data:
			return file_buffer_data.on_query_completions(view, prefix, locations)

	def on_activated(self, view):
		EventListener._ensureExistenceOfCompilationDatabase(view)

	@classmethod
	def _ensureExistenceOfCompilationDatabase(cls, view):
		filename = view.file_name()
		if not filename or not has_correct_extension(filename):
			return False
		window = view.window()
		if window.id() not in cls.activeWindows:
			cls.activeWindows.add(window.id())
			cls._loadCompilationDatabase(window)
		return True

	@classmethod
	def _loadCompilationDatabase(cls, window):
		try:
			project = window.project_data()
			if project is None: raise Exception('No sublime-project found for window {}.'.format(window.id()))
			cmakeSettings = project.get('cmake')
			if cmakeSettings is None: raise Exception('No cmake settings found for "{}".'.format(window.project_file_name()))
			cmakeSettings = sublime.expand_variables(cmakeSettings, window.extract_variables())
			buildFolder = cmakeSettings.get('build_folder')
			if buildFolder is None:
				raise KeyError('No "build_folder" key present in "cmake" settings of  "{}".'.format(window.project_file_name()))
			else:
				compilationDatabase = CompilationDatabase(buildFolder)
				cls.compilationDatabases[window.id()] = compilationDatabase
				clara_print('Loaded compilation database for window {}, located in "{}"'.format(window.id(), buildFolder))
		except Exception as e:
			clara_print('Window {}: {}'.format(window.id(), str(e)))

	def _get_file_buffer_data_for_view(self, view):
		file_name = view.file_name()
		if view.is_scratch() or not file_name:
			clara_print('View {}, {} is a scratch or has no filename.'.format(view.id(), file_name))
			return None
		elif file_name in self.file_buffers:
			file_buffer_data = self.file_buffers[file_name]
			file_buffer_data.add_view(view)
			return file_buffer_data
		elif has_correct_extension(file_name):
			fresh_file_buffer = FileBufferData(view)
			self.file_buffers[file_name] = fresh_file_buffer
			clara_print('Returning fresh FileBufferData')
			return fresh_file_buffer
		else:
			return None

	def _remove_view_from_file_buffers(self, view):
		file_buffer_data = self._get_file_buffer_data_for_view(view)
		if file_buffer_data:
			file_buffer_data.remove_view(view)
			# Keep it loaded or remove the FileBufferData as well?
			if not file_buffer_data.views:
				self.file_buffers.pop(file_buffer_data.file_name, None)

	def _ensure_view_is_loaded(self, view):
		if self._get_file_buffer_data_for_view(view):
			clara_print('Loaded {}, {}'.format(view.id(), view.file_name()))
			return True
		else:
			clara_print('Did not load {}, {}'.format(view.id(), view.file_name()))
			return False