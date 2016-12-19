import sublime, sublime_plugin

class ClaraDiagnoseCommand(sublime_plugin.ApplicationCommand):

	def run(self):
		view = sublime.active_window().new_file()
		view.set_scratch(True)
		view.set_name('Clara Diagnosis')
		view.run_command('clara_insert_diagnosis')
		view.set_read_only(True)
		sublime.active_window().focus_view(view)