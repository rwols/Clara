import sublime, getpass, socket
from .Functions import *
from .ViewData import *
from .ProgressIndicator import *

g_loaded_headers_atleast_once = False

class FileBufferData(object):
	"""We need data per buffer, not per view, so this class exists for that
	purpose."""

	def __init__(self, initial_view):
		super(FileBufferData, self).__init__()
		self.file_name = initial_view.file_name()
		assert is_header_or_implementation_file(self.file_name)
		self.session = None
		self.session_is_loading = True
		self.is_reparsing = False
		self.views = { initial_view.id(): ViewData(initial_view) }
		# FIXME: Make do without the below member variable.
		self.initial_view = initial_view 
		self.completions = None
		self.point = -1
		thread = threading.Thread(target=self._initialize_session)
		basename = os.path.basename(self.file_name)
		ProgressIndicator(thread, self.file_name, 
			"Parsing {}".format(basename), "Parsed {}".format(basename))
		thread.start()

	def add_view(self, new_view):
		if not self.has_view(new_view):
			self.views[new_view.id()] = ViewData(new_view)

	def has_view(self, view):
		return view.id() in self.views

	def remove_view(self, view):
		self.views.pop(view.id(), None)
		# FIXME: Loading an ASTUnit via an AST file results in assertion failures.
		if not self.views:
			try:
				self.session.save()
			except Exception as e:
				pass

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
			thread = threading.Thread(target=self._reparse)
			basename = os.path.basename(self.file_name)
			ProgressIndicator(thread, self.file_name, 
				"Reparsing {}".format(basename), "Reparsed {}".format(basename))
			thread.start()

	def on_query_completions(self, view, prefix, locations):
		assert view.id() in self.views
		if (not self.session or 
			self.session_is_loading or 
			self.is_reparsing or 
			len(locations) > 1):
			return None
		point = locations[0]
		if self.point == point and self.completions is not None:
			completions = self.completions
			self.completions = None
			self.point = -1
			return sublime_completion_tuple(completions)
		if not view.match_selector(point, 'source.c++'):
			return None
		if '-' in prefix and not '->' in prefix:
			return None
		if '>' in prefix and not '->' in prefix:
			return None
		if ':' in prefix and not '::' in prefix:
			return None
		clara_print('new auto-completion run at point {} with prefix {}'
			.format(point, prefix))
		row, col = sublime_point_to_clang_rowcol(view, point)
		unsaved_buffer = view.substr(sublime.Region(0, point))
		self.session.codeCompleteAsync(view.id(), unsaved_buffer, row, 
			col, None)

	def _diagnostic_callback(self, file_name, severity, row, column, message):
		diag_message = "{}: {}:{}:{}: {}".format(
			severity, file_name, str(row), str(column), message)
		clara_print(diag_message)
		if file_name != '' and file_name != self.file_name:
			return
		div_class = None
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
		for view_id, view_data in self.views.items():
			point = 0
			layout = 0
			print(view_data.view.name(), view_data.view.file_name())
			if row != -1 or column != -1:
				assert view_data
				assert view_data.view
				assert isinstance(view_data.view, sublime.View)
				point = view_data.view.text_point(row - 1, col - 1)
				print('point: {}'.format(point))
				point = max(0, point - 2)
				layout = sublime.LAYOUT_BELOW
			else:
				layout = sublime.LAYOUT_BLOCK
			region = sublime.Region(point, point)
			message = replace_single_quotes_by_tag(message, 'b')
			message = '<body id="Clara"><div id="diagnostic" class="{}">{}</div></body>'.format(div_class, message)
			phantom = sublime.Phantom(region, message, layout)
			view_data.new_diagnostics.append(phantom)
			view_data.active_diagnostics.update(view_data.new_diagnostics)

	def _completion_callback(self, view_id, row, col, completions):
		view = self.views[view_id].view
		self.point = clang_rowcol_to_sublime_point(view, row, col)
		if sublime.active_window().active_view() != view:
			# Too late, user is not interested anymore.
			return
		if len(completions) == 0:
			self.completions = None
			self.point = -1
			return
		self.completions = completions
		view.run_command('hide_auto_complete')
		view.run_command('auto_complete', {
			'disable_auto_insert': True,
			'api_completions_only': True,
			'next_compeltion_if_showing': True})

	def _load_headers(self):
		username = getpass.getuser()
		hostname = socket.gethostname()
		key = '{}@{}'.format(username, hostname)
		settings = sublime.load_settings(g_CLARA_SETTINGS)
		headers = settings.get(key)
		global g_loaded_headers_atleast_once
		if headers is not None:
			return headers
		elif not g_loaded_headers_atleast_once:
			g_loaded_headers_atleast_once = True
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
		if is_header_file(self.file_name):
			# Not yet supported.
			clara_print('Received header file, but that is not yet supported.')
			return
		elif not is_implementation_file(self.file_name):
			# The file doesn't have the correct extension, so forget about it.
			return
		compdb = get_compilation_database_for_view(self.initial_view)
		if not compdb:
			self.set_status('[!!!] This file has no compilation database [!!!]')
			sublime.set_timeout_async(self.erase_status, 10000)
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
		options.builtinHeaders = os.path.join(sublime.packages_path(), 'Clara', 'include')
		headers_dict = self._load_headers()
		if headers_dict:
			options.systemHeaders = headers_dict['system_headers']
			options.frameworks = headers_dict['system_frameworks']
		else:
			options.systemHeaders = ['']
			options.frameworks = ['']

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
		settings = sublime.load_settings(g_CLARA_SETTINGS)

		options.codeCompleteIncludeMacros = settings.get(
			'include_macros', True)

		options.codeCompleteIncludeCodePatterns = settings.get(
			'include_code_patterns', True)

		options.codeCompleteIncludeGlobals = settings.get(
			'include_globals', True)

		options.codeCompleteIncludeBriefComments = settings.get(
			'include_brief_comments', True)

		try: # Things may go wrong here.
			self.session = Session(options)
			self.session_is_loading = False # Success
			clara_print('Loaded "{}".'.format(self.file_name))
		except ASTFileReadError as e:
			clara_print(str(e))
			# Remove the cached file and try again.
			os.remove(options.astFile)
			self.session = Session(options)
			self.session_is_loading = False # Success
			clara_print('Loaded "{}".'.format(self.file_name))
		except ASTParseError as e:
			clara_print(str(e))

		decls = self.session.visitLocalDeclarations()
		print(decls)

	def _reparse(self):
		clara_print('Reparsing "{}"'.format(self.file_name))
		self.is_reparsing = True
		if not self.session.reparse():
			clara_print('Error occured during parsing of "{}"! This session will be disabled.'.format(self.file_name))
			self.is_reparsing = False
			self.session = None
			self.session_is_loading = True
		self.is_reparsing = False
		clara_print('Reparsed "{}"'.format(self.file_name))
