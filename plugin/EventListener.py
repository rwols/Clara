import sublime, sublime_plugin, os, time, datetime, threading
from .utils.FileBufferData import *

def plugin_loaded():
	verify_version()
	verify_platform()
	claraInitialize()

class EventListener(sublime_plugin.EventListener):

	def __init__(self):
		super(EventListener, self).__init__()
		self.file_buffers = {}
		clara_print('initialized EventListener')

	def on_new(self, view):
		if ensure_compilation_database_exists_for_view(view):
			if not self._ensure_view_is_loaded(view):
				clara_print('Did not load {}, {}'
					.format(view.id(), view.file_name()))

	def on_clone(self, view):
		self.on_new(view)

	def on_load(self, view):
		self.on_new(view)

	def on_activated(self, view):
		self.on_new(view)

	def on_close(self, view):
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
		ensure_compilation_database_exists_for_view(view)

	def on_post_window_command(self, window, command_name, args):
		clara_print('Got window command: ' + command_name)
		if command_name == 'cmake_configure':
			def f():
				clara_print('Reloading compilation database for window {}'.format(window.id())) 
				load_compilation_database_for_window(window)
			sublime.set_timeout(f, 1000)
			
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
