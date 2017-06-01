from ..utils.Functions import ensure_compilation_database_exists_for_view
import sublime_plugin, sublime

class CompilationDatabaseWatcher(sublime_plugin.EventListener):

    def on_new(self, view):
        ensure_compilation_database_exists_for_view(view)

    on_activated = on_load = on_clone = on_new

    def on_post_save(self, view):
        listeners = sublime_plugin.view_event_listeners.get(view.id(), None)
        if not listeners:
            return
        for listener in listeners:
            try:
                listener.on_post_save()
            except Exception as e:
                continue
                
