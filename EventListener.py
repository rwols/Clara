import sublime, sublime_plugin
import os, sys
CURRENT_DIR = os.path.dirname(os.path.abspath(__file__))
if CURRENT_DIR not in sys.path: sys.path.append(CURRENT_DIR)
import cpp as Clara

class EventListener(sublime_plugin.EventListener):
	"""Manages a project object and dispatches session objects to new views."""

	def __init__(self):
		"""Initializes the project object."""
		super(EventListener, self).__init__()


	
	
