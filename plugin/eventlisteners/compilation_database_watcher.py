import sublime_plugin
import Clara.Clara
from .code_completer import CodeCompleter

class CompilationDatabaseWatcher(sublime_plugin.EventListener, 
                                 Clara.Clara.CompilationDatabaseWatcher):

    def on_post_save(self, view):
        listeners = sublime_plugin.view_event_listeners.get(view.id(), [])
        for listener in listeners:
            if isinstance(listener, CodeCompleter):
                listener.on_post_save()
