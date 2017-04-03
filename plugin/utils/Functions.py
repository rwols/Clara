import sublime, os, time, datetime, threading, html
from .Globals import *
from ..Clara import *

def verify_version():
	if claraVersion() > int(sublime.version()):
		sublime.error_message('Your Sublime version is too low for Clara. '
			'It must be at least version {}, while your Sublime version is {}.'
			.format(claraVersion(), int(sublime.version())))

def verify_platform():
	if claraPlatform() != sublime.platform():
		sublime.error_message("Clara platform mismatch. "
			"Sublime's platform is {} while Clara's platform is {}."
			.format(sublime.platform(), claraPlatform()))

def clara_print(*messages):
	if sublime.active_window().active_view().settings().get('clara_debug', False):
		with g_printer_lock as lock:
			print('clara:', *messages)

def is_implementation_file(file_name):
	extension = os.path.splitext(file_name)[1]
	return extension in ['.cpp', '.cc', '.c', '.cxx']

def is_header_file(file_name):
	extension = os.path.splitext(file_name)[1]
	return extension in ['.hpp', '.hh', '.h', '.hxx']

def is_header_or_implementation_file(file_name):
	return is_implementation_file(file_name) or is_header_file(file_name)

def get_compilation_database_for_view(view):
	try:
		ensure_compilation_database_exists_for_view(view)
		database = g_compilation_databases[view.window().id()]
		return database
	except KeyError as keyError:
		clara_print('No database found.')
		return None
	except AttributeError as attrError:
		clara_print('View has no window anymore.')
		return None

def ensure_compilation_database_exists_for_view(view):
	file_name = view.file_name()
	if not file_name or not is_implementation_file(file_name):
		return False
	window = view.window()
	if window is None:
		return False
	if window.id() not in g_active_windows:
		g_active_windows.add(window.id())
		load_compilation_database_for_window(window)
	return True

def load_compilation_database_for_window(window):
	try:
		settings = window.active_view().settings()
		compile_commands = settings.get('compile_commands')
		compile_commands = sublime.expand_variables(compile_commands, window.extract_variables())
		# project = window.project_data()
		# if project is None: raise Exception(
		# 	'No sublime-project found for window {}.'.format(window.id()))
		# cmake = project.get('cmake')
		# if cmake is None: raise Exception(
		# 	'No cmake settings found for "{}".'.format(
		# 		window.project_file_name()))
		# cmake = sublime.expand_variables(
		# 	cmake, window.extract_variables())
		# build_folder = cmake.get('build_folder')
		if not compile_commands:
			raise KeyError(
				'No "compile_commands" key present in settings.'
				.format(window.project_file_name()))
		else:
			compdb = CompilationDatabase(compile_commands)
			g_compilation_databases[window.id()] = compdb
			clara_print('Loaded compilation database for window {}, located'
			' in "{}"'.format(window.id(), compile_commands))
	except Exception as e:
		clara_print('Window {}: {}'.format(window.id(), str(e)))

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
	settings = sublime.load_settings(g_CLARA_SETTINGS)
	iwc = settings.get('inhibit_word_completions', True)
	iec = settings.get('inhibit_explicit_completions', True)
	iwc = sublime.INHIBIT_WORD_COMPLETIONS if iwc else 0
	iec = sublime.INHIBIT_EXPLICIT_COMPLETIONS if iec else 0
	return (completions, iwc | iec)
