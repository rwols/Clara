import sublime, os, threading, html
import Clara.Clara

def verify_version():
	if Clara.Clara.version() > int(sublime.version()):
		sublime.error_message('Your Sublime version is too low for Clara. '
			'It must be at least version {}, while your Sublime version is {}.'
			.format(Clara.Clara.version(), int(sublime.version())))

def verify_platform():
	if Clara.Clara.platform() != sublime.platform():
		sublime.error_message("Clara platform mismatch. "
			"Sublime's platform is {} while Clara's platform is {}."
			.format(sublime.platform(), Clara.Clara.platform()))

def clara_print(*messages):
	if sublime.active_window().active_view().settings().get("clara_debug", False):
		msg = ""
		for message in messages:
			if isinstance(message, str):
				msg += message + " "
			else:
				msg += str(message) + " "
		sublime.status_message(msg)
		print("clara:", *messages)

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
	settings = sublime.load_settings("Clara.sublime-settings")
	iwc = settings.get('inhibit_word_completions', True)
	iec = settings.get('inhibit_explicit_completions', True)
	iwc = sublime.INHIBIT_WORD_COMPLETIONS if iwc else 0
	iec = sublime.INHIBIT_EXPLICIT_COMPLETIONS if iec else 0
	return (completions, iwc | iec)
