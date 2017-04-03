from .code_completer import CodeCompleter
from ..utils.Functions import ensure_compilation_database_exists_for_view
import sublime_plugin

class CompilationDatabaseWatcher(sublime_plugin.EventListener):

    def on_new(self, view):
        ensure_compilation_database_exists_for_view(view)

    def on_clone(self, view):
        self.on_new(view)

    def on_load(self, view):
        self.on_new(view)

    def on_activated(self, view):
        self.on_new(view)

    def on_post_save(self, view):
        listeners = sublime_plugin.view_event_listeners.get(view.id(), None)
        if not listeners:
            return
        for listener in listeners:
            if isinstance(listener, CodeCompleter):
                listener.on_post_save()
