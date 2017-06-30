import sublime_plugin
import Clara.Clara

class CompilationDatabaseWatcher(sublime_plugin.EventListener, 
                                 Clara.Clara.CompilationDatabaseWatcher):

    def on_post_save(self, view):
        listeners = sublime_plugin.view_event_listeners.get(view.id(), None)
        if not listeners:
            return
        for listener in listeners:
            if hasattr(listener, "on_post_save"):
                listener.on_post_save()
                
