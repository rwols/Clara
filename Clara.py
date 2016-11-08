import sublime
import sublime_plugin

import os   # for path handling
import imp  # for loading the dynamic library


pwd = os.path.dirname(os.path.abspath(__file__))
claraPath = os.path.join(pwd, 'build/lib/Clara.so')
Clara = imp.load_dynamic('Clara', claraPath)
session = Clara.Session()

def setStatus(msg):
	sublime.active_window().status_message(msg)

class ClaraListener(sublime_plugin.EventListener):

	def __init__(self):
		super(ClaraListener, self).__init__()
		active = sublime.active_window()
		data = active.project_data()
		projectName = active.project_file_name()
		projectPath = os.path.dirname(projectName)
		bfolder = 'build_folder'
		buildPath = data[bfolder] if bfolder in data else os.path.join(projectPath, 'build')
		compileDefsPath = os.path.join(buildPath, 'compile_commands.json')
		errorMessage = session.loadCompilationDatabase(compileDefsPath)
		if not session.hasCompilationDatabase():
			sublime.error_message('Unable to load JSON compilation database at ' + compileDefsPath)

	def codeCompleteAt(self, filename, buffer, prefix, row, col):
		session.setSourcePaths([filename])
		msg = 'code completing ' + prefix + ' in file ' + filename + ':' + str(row) + ':' + str(col)
		setStatus(msg)
		completion = session.codeComplete(filename, buffer, row, col)
		return completion
	
	def on_load_async(self, view):
		if not self.viewIsRelevant(view): return
		filename = view.file_name()
		if filename is None: return
		print('on_load_async: ' + filename)

	def on_post_save_async(self, view):
		
		if not self.viewIsRelevant(view): return
		filename = view.file_name()
		if filename is None: return
		print('on_post_save_async: ' + filename)

	def on_selection_modified_async(self, view):
		if not self.viewIsRelevant(view): return
		filename = view.file_name()
		if filename is None: return
		print('on_selection_modified_async: ' + filename)

	def on_activated_async(self, view):
		if not self.viewIsRelevant(view): return
		filename = view.file_name()
		if filename is None: return
		print('on_activated_async: ' + filename)

	def on_deactivated_async(self, view):
		if not self.viewIsRelevant(view): return
		filename = view.file_name()
		if filename is None: return
		print('on_deactivated_async: ' + filename)

	def on_hover(self, view, point, hover_zone):
		if not self.pointIsRevelant(view, point): return
		filename = view.file_name()
		if filename is None: return
		row, column = view.rowcol(point)
		print('on_hover: ' + filename + ':' + str(row) + ':' + str(column))

	def pointIsRevelant(self, view, point):
		return view.match_selector(point, 'source.c++')

	def viewIsRelevant(self, view):
		syntax = view.settings().get('syntax')
		return 'C++' in syntax

	def getBuffer(self, view):
		return view.substr(sublime.Region(0, view.size()))

	def getFilenameAndBufferRowCol(self, view, point):
		buffer = self.getBuffer(view)
		row, col = view.rowcol(point)
		return view.file_name(), buffer, row, col

	def sublimeRowCol2ClangRowCol(self, row, col):
		return row + 1, col + 1

	def on_query_completions(self, view, prefix, locations):
		completions = [] # Start with an empty list.
		for point in locations:
			if not self.pointIsRevelant(view, point): continue
			line = view.substr(view.line(point))
			numTabs = line.count('\t')
			tabSize = view.settings().get('tab_size')
			filename, buffer, row, column = self.getFilenameAndBufferRowCol(view, point)
			print('sublime ({0},{1})'.format(row, column))
			row, column = self.sublimeRowCol2ClangRowCol(row, column)
			print('clang ({0},{1})'.format(row, column))
			completion = self.codeCompleteAt(filename, buffer, prefix, row, column)
			completions.extend(completion)
		print(completions)
		return completions

class RenameFunctionCommand(sublime_plugin.TextCommand):

	def onDone(self, userInput):
		setStatus(userInput)

	def onChange(self, userInput):
		setStatus(userInput)

	def onCancel(self):
		setStatus('user wimped out')

	def run(self, edit):
		active = sublime.active_window()
		caption = 'Rename Function: New Name'
		initialText = 'new fully qualified name'
		active.show_input_panel(caption, initialText, self.onDone, self.onChange, self.onCancel)

	def is_visible(self):
		return True