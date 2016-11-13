import sublime, sublime_plugin
from .cpp import *

class EventListener(sublime_plugin.EventListener):
	"""Manages a project object and dispatches session objects to new views."""

	def __init__(self):
		"""Initializes the project object."""
		super(EventListener, self).__init__()


	
	
