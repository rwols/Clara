import sublime_plugin

import Clara.Clara
class CodeCompleter(sublime_plugin.ViewEventListener, Clara.Clara.CodeCompleter):

    @classmethod
    def is_applicable(cls, settings):
        return settings.get("_clara_code_completer", False)

    def __init__(self, view):
        sublime_plugin.ViewEventListener.__init__(self, view)
        Clara.Clara.CodeCompleter.__init__(self, view)

    def on_query_completions(self, prefix, locations):
        return Clara.Clara.CodeCompleter.on_query_completions(self, prefix, locations)

