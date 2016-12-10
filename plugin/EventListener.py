import sublime, sublime_plugin, os, time, datetime, threading
from .Clara import *

_printLock = threading.Lock()

def claraPrint(message):
	if sublime.load_settings('Clara.sublime-settings').get('debug', False):
		t = datetime.datetime.fromtimestamp(time.time()).strftime('%X')
		_printLock.acquire()
		print('Clara:{}: {}'.format(t, message))
		_printLock.release()

def hasCorrectExtension(filename):
	return os.path.splitext(filename)[1] in ['.cpp', '.cc', '.c', '.cxx', '.objc']

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
			claraPrint('No database found.')
			return None
		except AttributeError as attrError:
			claraPrint('View has no window anymore.')
			return None

	def __init__(self):
		super(EventListener, self).__init__()

	def on_load_async(self, view):
		EventListener._ensureExistenceOfCompilationDatabase(view)

	def on_activated_async(self, view):
		EventListener._ensureExistenceOfCompilationDatabase(view)

	@classmethod
	def _ensureExistenceOfCompilationDatabase(cls, view):
		filename = view.file_name()
		if not filename:
			return
		if not hasCorrectExtension(filename):
			return
		window = view.window()
		if window.id() not in cls.activeWindows:
			cls.activeWindows.add(window.id())
			cls._loadCompilationDatabase(window)

	@classmethod
	def _loadCompilationDatabase(cls, window):
		try:
			project = window.project_data()
			if project is None: raise Exception('No sublime-project found for window {}.'.format(window.id()))
			cmakeSettings = project.get('cmake')
			if cmakeSettings is None: raise Exception('No cmake settings found in sublime-project file of window {}.'.format(window.id()))
			cmakeSettings = sublime.expand_variables(cmakeSettings, window.extract_variables())
			buildFolder = cmakeSettings.get('build_folder')
			if buildFolder is None:
				raise KeyError('No "build_folder" key present in "cmake" settings of window {}.'.format(window.id()))
			else:
				compilationDatabase = CompilationDatabase(buildFolder)
				cls.compilationDatabases[window.id()] = compilationDatabase
				claraPrint('Loaded compilation database for window {}.'.format(window.id()))
		except Exception as e:
			claraPrint('Window {}: {}'.format(window.id(), str(e)))
