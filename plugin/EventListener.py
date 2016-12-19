import sublime, sublime_plugin, os, time, datetime, threading
from .Clara import *
from inspect import currentframe, getframeinfo
from .utils.ProgressIndicator import ProgressIndicator

_printer_lock = threading.Lock()
_loaded_headers_atleast_once = False
_CLARA_SETTINGS = 'Clara.sublime-settings'

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

def clara_print(message):
	if sublime.load_settings(_CLARA_SETTINGS).get('debug', True):
		with _printer_lock as lock:
			t = datetime.datetime.fromtimestamp(time.time()).strftime('%X')
			print('Clara:{}: {}'.format(t, message))

def is_implementation_file(file_name):
	extension = os.path.splitext(file_name)[1]
	return extension in ['.cpp', '.cc', '.c', '.cxx']

def is_header_file(file_name):
	extension = os.path.splitext(file_name)[1]
	return extension in ['.hpp', '.hh', '.h', '.hxx']

def is_header_or_implementation_file(file_name):
	return is_implementation_file(file_name) or is_header_file(file_name)

def sublime_point_to_clang_rowcol(view, point):
	row, col = view.rowcol(point)
	return (row + 1, col + 1)

def clang_rowcol_to_sublime_point(view, row, col):
	return view.text_point(row - 1, col - 1)

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

def sublime_completion_tuple(completions):
	settings = sublime.load_settings(_CLARA_SETTINGS)
	iwc = settings.get('inhibit_word_completions', True)
	iec = settings.get('inhibit_explicit_completions', True)
	iwc = sublime.INHIBIT_WORD_COMPLETIONS if iwc else 0
	iec = sublime.INHIBIT_EXPLICIT_COMPLETIONS if iec else 0
	return (completions, iwc | iec) 

class ViewData(object):
	"""ViewData"""
	def __init__(self, view):
		super(ViewData, self).__init__()
		self.view = view
		self.active_diagnostics = sublime.PhantomSet(self.view)
		self.new_diagnostics = []

class FileBufferData(object):
	"""FileBufferData"""
	def __init__(self, initial_view):
		super(FileBufferData, self).__init__()
		self.file_name = initial_view.file_name()
		assert is_header_or_implementation_file(self.file_name)
		self.session = None
		self.session_is_loading = True
		self.is_reparsing = False
		self.views = { initial_view.id(): ViewData(initial_view) }
		# self.views[initial_view.id()] = ViewData(initial_view)
		self.initial_view = initial_view # FIXME: Make do without this member variable
		self.point_to_completions = {}
		self.inflight_completions = set()
		thread = threading.Thread(target=self._initialize_session)
		thread.start()
		basename = os.path.basename(self.file_name)
		ProgressIndicator(thread, self.file_name, "Parsing {}".format(basename), "Parsed {}".format(basename))
		# threading.Thread(target=self._initialize_session).start()

	def add_view(self, new_view):
		if not self.has_view(new_view):
			self.views[new_view.id()] = ViewData(new_view)

	def has_view(self, view):
		return view.id() in self.views

	def remove_view(self, view):
		self.views.pop(view.id(), None)
		# FIXME: Loading an ASTUnit via an AST file results in assertion failures.
		# if not self.views:
		# 	try:
		# 		self.session.save()
		# 	except Exception as e:
		# 		pass

	def set_status(self, message):
		for view_id, view_data in self.views.items():
			view_data.view.set_status('Clara', message)

	def erase_status(self):
		for view_id, view_data in self.views.items():
			view_data.view.erase_status('Clara')

	def on_post_save(self):
		if (self.session and 
			not self.session_is_loading and 
			not self.is_reparsing):
			threading.Thread(target=self._reparse).start()

	def on_query_completions(self, view, prefix, locations):
		assert view.id() in self.views
		if (not self.session or 
			self.session_is_loading or 
			self.is_reparsing or 
			len(locations) > 1):
			return None
		point = locations[0]
		if not view.match_selector(point, 'source.c++'):
			return None
		if '-' in prefix and not '->' in prefix:
			# Not sure why this triggers.
			# FIXME: This is hacky, find out why this triggers.
			return None
		if ':' in prefix:
			# Not sure why this triggers.
			# FIXME: This is hacky, find out why this triggers.
			return None
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
				return sublime_completion_tuple(completions)
		else:
			if point in self.inflight_completions:
				clara_print('Already completing at point {}'.format(point))
				return None
			else:
				clara_print('Starting new auto-completion run at point {} with prefix {}'
					.format(point, prefix))
				self.inflight_completions.add(point)
				row, col = sublime_point_to_clang_rowcol(view, point)
				unsaved_buffer = view.substr(sublime.Region(0, point))
				self.session.codeCompleteAsync(view.id(), unsaved_buffer, row, col, self._completion_callback)

	def _diagnostic_callback(self, file_name, severity, row, column, message):
		DIAG_PANEL_NAME = 'Diagnostics'
		frameinfo = getframeinfo(currentframe())
		print(frameinfo.filename, frameinfo.lineno)
		# active_view = sublime.active_window().active_view().id()
		# diag_view = sublime.create_output_panel(DIAG_PANEL_NAME)
		diag_message = "{}: {}:{}:{}: {}".format(
			severity, file_name, str(row), str(column), message)
		clara_print(diag_message)
		frameinfo = getframeinfo(currentframe())
		print(frameinfo.filename, frameinfo.lineno)
		if file_name != '' and file_name != self.file_name:
			# diag_view.run_command('insert', {'characters': diag_message})
			return
		frameinfo = getframeinfo(currentframe())
		print(frameinfo.filename, frameinfo.lineno)
		view_data = self.views[active_view.id()]
		div_class = None
		frameinfo = getframeinfo(currentframe())
		print(frameinfo.filename, frameinfo.lineno)
		if severity == 'begin':
			# clara_print('BEGIN DIAGNOSIS'.format(file_name))
			view_data.new_diagnostics = []
			view_data.active_diagnostics.update(view_data.new_diagnostics)
			# sublime.destroy_output_panel(DIAG_PANEL_NAME)
			return
		elif severity == 'finish':
			# clara_print('END DIAGNOSIS'.format(file_name))
			# view_data.new_diagnostics = []
			# view_data.active_diagnostics.update(view_data.new_diagnostics)
			return
		elif severity == 'warning':
			div_class = 'warning'
		elif severity == 'error' or severity == 'fatal':
			div_class = 'error'
		elif severity == 'note':
			div_class = 'inserted'
		elif severity == 'remark':
			div_class = 'inserted'
		else:
			div_class = 'inserted'
		frameinfo = getframeinfo(currentframe())
		print(frameinfo.filename, frameinfo.lineno)
		point = 0
		if row != -1 or column != -1:
			point = clang_rowcol_to_sublime_point(view_data.view, row, col)
			# The reason is not clear, but Clang somehow puts error too much to
			# the right (two character points to be precise), so we subtract
			# that here again to make the phantoms line up at the exact error.
			point = max(0, point - 2)
		region = sublime.Region(point, point)
		message = replace_single_quotes_by_tag(message, 'b')
		message = '<body id="Clara"><div id="diagnostic" class="{}">{}</div></body>'.format(div_class, message)
		phantom = sublime.Phantom(region, message, sublime.LAYOUT_BELOW)
		view_data.new_diagnostics.append(phantom)
		frameinfo = getframeinfo(currentframe())
		print(frameinfo.filename, frameinfo.lineno)
		view_data.active_diagnostics.update(view_data.new_diagnostics)

	def _completion_callback(self, view_id, row, col, completions):
		view = self.views[view_id].view
		point = clang_rowcol_to_sublime_point(view, row, col)
		self.inflight_completions.discard(point)
		if sublime.active_window().active_view() != view:
			# Too late, user is not interested anymore.
			return
		if point in self.point_to_completions:
			clara_print('point {} is already in completion dictionary. '
				.format(point))
			return
		clara_print('adding point {}'.format(point))
		if len(completions) == 0:
			completions = 'FOUND NOTHING'
		else:
			self.point_to_completions[point] = completions
			view.run_command('hide_auto_complete')
			view.run_command('auto_complete', {
				'disable_auto_insert': True,
				'api_completions_only': False,
				'next_competion_if_showing': True})

	def _load_headers(self, key):
		settings = sublime.load_settings(_CLARA_SETTINGS)
		headers = settings.get(key)
		global _loaded_headers_atleast_once
		if headers is not None:
			return headers
		elif not _loaded_headers_atleast_once:
			_loaded_headers_atleast_once = True
			if sublime.ok_cancel_dialog('You do not yet have headers set up. '
				'Do you want to generate them now?'):
				sublime.run_command('generate_system_headers')
				return settings.get(key)
			else:
				sublime.error_message(
					'Clara will not work properly without setting up headers!')
				return None
		else:
			return None

	def _initialize_session(self):
		if not is_implementation_file(self.file_name):
			# The file doesn't have the correct extension, so forget about it.
			return
		compdb = EventListener.get_compilation_database_for_view(
			self.initial_view)
		if not compdb:
			# Not even going to try.
			return

		# Start constructing the SessionOptions object in order to construct a
		# Session object.
		options = SessionOptions()
		options.invocation, options.workingDirectory = compdb.get(
			self.file_name)
		if not options.invocation:
			# This file is not in the compilation database, so forget about it.
			clara_print('The file "{}" is not in the compilation database, '
				'so we are not doing code-completion.'.format(self.file_name))
			return

		# Load in the system headers and builtin headers.
		headers = self._load_headers('system_headers')
		frameworks = self._load_headers('system_frameworks')
		options.systemHeaders = [""] if headers is None else headers
		options.frameworks = '' if frameworks is None else frameworks

		# At this point we can't really fail, so set the view's status to
		# something informative to let the user know that we are parsing.
		clara_print('Loading "{}" with working directory "{}" '
			'and compiler invocation {}'.format(self.file_name, 
				options.workingDirectory, str(options.invocation)))
		
		options.diagnosticCallback = self._diagnostic_callback
		options.logCallback = clara_print
		options.codeCompleteCallback = self._completion_callback

		options.filename = self.file_name

		# AST file handling.
		ast_dir = os.path.join(options.workingDirectory, ".clara")
		options.astFile = os.path.join(ast_dir, 
			os.path.basename(self.file_name) + ".ast")
		clara_print('The AST working file will be "{}"'.format(options.astFile))
		os.makedirs(ast_dir, exist_ok=True)

		# Code completion options
		settings = sublime.load_settings(_CLARA_SETTINGS)

		options.codeCompleteIncludeMacros = settings.get(
			'include_macros', True)

		options.codeCompleteIncludeCodePatterns = settings.get(
			'include_code_patterns', True)

		options.codeCompleteIncludeGlobals = settings.get(
			'include_globals', True)

		options.codeCompleteIncludeBriefComments = settings.get(
			'include_brief_comments', True)

		self.session = Session(options)
		self.session_is_loading = False # Success
		clara_print('Loaded "{}"'.format(self.file_name))
		# self.erase_status()

	def _reparse(self):
		clara_print('Reparsing {}'.format(self.file_name))
		self.set_status('Reparsing, this can take a while!')
		self.is_reparsing = True
		self.session.reparse(0, self._reparse_callback)
		

	def _reparse_callback(self, view_id, success):
		self.is_reparsing = False
		self.erase_status()
		if success:
			clara_print('Reparsed "{}"'.format(self.file_name))
		else:
			clara_print('Error occured while reparsing "{}"'
				.format(self.file_name))
			self.session = None
			self.session_is_loading = True


class EventListener(sublime_plugin.EventListener):

	compilationDatabases = {}
	activeWindows = set()

	@classmethod
	def get_compilation_database_for_view(cls, view):
		try:
			cls._ensure_compilation_database_exists_for_view(view)
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
		clara_print('initialized EventListener')

	def on_new(self, view):
		clara_print('on_new: {}, {}'.format(view.id(), view.file_name()))
		if EventListener._ensure_compilation_database_exists_for_view(view):
			if not self._ensure_view_is_loaded(view):
				clara_print('Did not load {}, {}'
					.format(view.id(), view.file_name()))

	def on_clone(self, view):
		# clara_print('on_clone: {}, {}'.format(view.id(), view.file_name()))
		self.on_new(view)

	def on_load(self, view):
		# clara_print('on_load: {}, {}'.format(view.id(), view.file_name()))
		self.on_new(view)

	def on_activated(self, view):
		# clara_print('on_activated: {}, {}'.format(view.id(), view.file_name()))
		self.on_new(view)

	def on_close(self, view):
		# clara_print('on_close: {}, {}'.format(view.id(), view.file_name()))
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
		EventListener._ensure_compilation_database_exists_for_view(view)

	@classmethod
	def _ensure_compilation_database_exists_for_view(cls, view):
		file_name = view.file_name()
		if not file_name or not is_implementation_file(file_name):
			return False
		window = view.window()
		if window is None:
			return False
		if window.id() not in cls.activeWindows:
			cls.activeWindows.add(window.id())
			cls._load_compilation_database_for_window(window)
		return True

	@classmethod
	def _load_compilation_database_for_window(cls, window):
		try:
			project = window.project_data()
			if project is None: raise Exception(
				'No sublime-project found for window {}.'.format(window.id()))
			cmakeSettings = project.get('cmake')
			if cmakeSettings is None: raise Exception(
				'No cmake settings found for "{}".'.format(
					window.project_file_name()))
			cmakeSettings = sublime.expand_variables(
				cmakeSettings, window.extract_variables())
			buildFolder = cmakeSettings.get('build_folder')
			if buildFolder is None:
				raise KeyError(
					'No "build_folder" key present in "cmake" settings of "{}".'
					.format(window.project_file_name()))
			else:
				compilationDatabase = CompilationDatabase(buildFolder)
				cls.compilationDatabases[window.id()] = compilationDatabase
				clara_print('Loaded compilation database for window {}, located'
				' in "{}"'.format(window.id(), buildFolder))
		except Exception as e:
			clara_print('Window {}: {}'.format(window.id(), str(e)))

	def _get_file_buffer_data_for_view(self, view):
		file_name = view.file_name()
		if view.is_scratch() or not file_name:
			clara_print('View {}, {} is a scratch or has no file_name.'
				.format(view.id(), file_name))
			return None
		elif file_name in self.file_buffers:
			file_buffer_data = self.file_buffers[file_name]
			file_buffer_data.add_view(view)
			return file_buffer_data
		elif is_implementation_file(file_name):
			fresh_file_buffer = FileBufferData(view)
			self.file_buffers[file_name] = fresh_file_buffer
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
			clara_print('Did not load {}, {}'
				.format(view.id(), view.file_name()))
			return False