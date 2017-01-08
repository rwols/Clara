import threading

g_CLARA_SETTINGS = 'Clara.sublime-settings'
g_compilation_databases = {}
g_active_windows = set()
g_printer_lock = threading.Lock()
