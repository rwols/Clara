import sublime

class ViewData(object):
	"""ViewData"""
	def __init__(self, view):
		super(ViewData, self).__init__()
		assert view
		self.view = view
		self.active_diagnostics = sublime.PhantomSet(self.view)
		self.new_diagnostics = []