import sublime, sublime_plugin, subprocess, os

LAYOUT = { "cols": [0.0, 0.5, 1.0], "rows": [0.0, 1.0], "cells": [[0,0,1,1],[1,0,2,1]] }
USER_SETTINGS = { "file": "${packages}/User/Clara.sublime-settings" }
DEFAULT_SETTINGS = { "file": "${packages}/Clara/Clara.sublime-settings" }

class ClaraOpenSettingsCommand(sublime_plugin.ApplicationCommand):
	"""Opens a new window with the default settings on the left and the user 
	settings on the right."""

	def run(self):
		sublime.run_command('new_window')
		window = sublime.active_window()
		window.run_command('set_layout',LAYOUT)
		views = window.views()
		window.focus_group(1)
		window.run_command('open_file', USER_SETTINGS)
		window.focus_group(0)
		window.run_command('open_file', DEFAULT_SETTINGS)

	def description(self):
		return 'Opens default and user settings side-by-side.'
