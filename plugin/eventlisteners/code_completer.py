import sublime
import sublime_plugin
import threading
import getpass
import socket
from ..utils.Functions import *
from ..utils.ProgressIndicator import *

class CodeCompleter(sublime_plugin.ViewEventListener):

    loaded_headers_atleast_once = False

    @classmethod
    def is_applicable(cls, settings):
        return 'c++' in settings.get('syntax').lower()

    def __init__(self, view):
        super(CodeCompleter, self).__init__(view)
        self.session_is_loaded = False
        self.session = None
        self.point = -1
        self.completions = None
        self.phantom_set = sublime.PhantomSet(self.view)
        self.phantoms = []
        self._init_session_on_another_thread()

    def on_query_completions(self, prefix, locations):
        if not self.session_is_loaded or len(locations) != 1:
            return None
        point = locations[0]
        if self.point == point or self.point + 3 == point and self.completions:
            completions = self.completions
            self.completions = None
            self.point = -1
            return sublime_completion_tuple(completions)
        if not self.view.match_selector(point, 'source.c++'):
            return None
        if '-' in prefix and not '->' in prefix:
            return None
        if '>' in prefix and not '->' in prefix:
            return None
        if ':' in prefix and not '::' in prefix:
            return None
        clara_print('new auto-completion run at point', point, 'with prefix', prefix)
        row, col = sublime_point_to_clang_rowcol(self.view, point)
        unsaved_buffer = self.view.substr(sublime.Region(0, point))
        self.session.codeCompleteAsync(self.view.id(), unsaved_buffer, row, 
            col, None)

    def on_post_save(self):
        if not self.session:
            # Attempt to load a session now.
            # Maybe we had a temporary buffer, or maybe the compilation database
            # has been updated in the mean time. These issues may be fixed now.
            self._init_session_on_another_thread()
        elif self.session_is_loaded:
            thread = threading.Thread(target=self._reparse)
            basename = os.path.basename(self.view.file_name())
            ProgressIndicator(thread, self.view.file_name(), 
              "Reparsing {}".format(basename), "Reparsed {}".format(basename))
            thread.start()

    def _init_session_on_another_thread(self):
        thread = threading.Thread(target=self._init_session)
        basename = os.path.basename(self.view.file_name())
        ProgressIndicator(thread, self.view.file_name(), 
            "Parsing {}".format(basename), "Parsed {}".format(basename))
        thread.start()

    def _init_session(self):
        compdb = get_compilation_database_for_view(self.view)
        if not compdb:
            clara_print('no compilation database for', self.view.file_name())
            return
        options = SessionOptions()
        options.invocation, options.workingDirectory = compdb.get(self.view.file_name())
        options.builtinHeaders = os.path.join(sublime.packages_path(), 'Clara', 'include')
        headers_dict = self._load_headers()
        if headers_dict:
            options.systemHeaders = headers_dict['system_headers']
            options.frameworks = headers_dict['system_frameworks']
        else:
            clara_print('no headers!')
            return
        clara_print('loading', self.view.file_name(),'with working directory', 
            options.workingDirectory, 'and compiler invocation', 
            str(options.invocation))
        options.diagnosticCallback = self._diagnostic_callback
        options.logCallback = clara_print
        options.codeCompleteCallback = self._completion_callback
        options.filename = self.view.file_name()
        ast_dir = os.path.join(options.workingDirectory, ".clara")
        options.astFile = os.path.join(ast_dir, 
            os.path.basename(self.view.file_name()) + ".ast")
        clara_print('The AST working file will be', options.astFile)
        os.makedirs(ast_dir, exist_ok=True)
        settings = sublime.load_settings(g_CLARA_SETTINGS)

        options.codeCompleteIncludeMacros = settings.get(
            'include_macros', True)

        options.codeCompleteIncludeCodePatterns = settings.get(
            'include_code_patterns', True)

        options.codeCompleteIncludeGlobals = settings.get(
            'include_globals', True)

        options.codeCompleteIncludeBriefComments = settings.get(
            'include_brief_comments', True)

        try: # Things may go wrong here.
            self.session = Session(options)
            self.session_is_loaded = True # Success
            clara_print('loaded', self.view.file_name())
        except ASTFileReadError as e:
            clara_print(str(e))
            # Remove the cached file and try again.
            os.remove(options.astFile)
            self.session = Session(options)
            self.session_is_loaded = True # Success
            clara_print('loaded', self.view.file_name())
        except ASTParseError as e:
            clara_print(str(e))

    def _load_headers(self):
        username = getpass.getuser()
        hostname = socket.gethostname()
        key = '{}@{}'.format(username, hostname)
        settings = sublime.load_settings(g_CLARA_SETTINGS)
        headers = settings.get(key)
        if headers is not None:
            return headers
        elif not ViewEventListener.loaded_headers_atleast_once:
            ViewEventListener.loaded_headers_atleast_once = True
            if sublime.ok_cancel_dialog('You do not yet have headers set up. '
                'Do you want to generate them now?'):
                sublime.run_command('generate_system_headers')
                return settings.get(key)
            else:
                sublime.error_message(
                    'Clara will not work properly without setting up headers!')
                return None
        else:
            return None

    def _completion_callback(self, view_id, row, col, completions):
        if view_id != self.view.id():
            # This callback is intended for another view.
            # FIXME: This is not necessary anymore I believe.
            return
        if sublime.active_window().active_view() != self.view:
            # Too late, user is not interested anymore.
            return
        self.point = clang_rowcol_to_sublime_point(self.view, row, col)
        if len(completions) == 0:
            self.completions = None
            self.point = -1
            return
        self.completions = completions
        self.view.run_command('hide_auto_complete')
        self.view.run_command('auto_complete', {
            'disable_auto_insert': True,
            'api_completions_only': True,
            'next_completion_if_showing': True})

    def _diagnostic_callback(self, file_name, severity, row, column, message):
        if severity == 'begin':
            self.phantoms = []
            self.phantom_set.update(self.phantoms)
            return
        if file_name != self.view.file_name():
            return
        print(row, column, severity, message)
        point = self.view.text_point(row - 1, col - 1)
        region = sublime.Region(point, point)
        message = replace_single_quotes_by_tag(message, 'b')
        message = '<body id="Clara"><div id="diagnostic" class="{}">{}</div></body>'.format(div_class, message)
        phantom = sublime.Phantom(region, message, layout)
        self.phantoms.append(phantom)
        self.phantom_set.update(self.phantoms)

    def _reparse(self):
        clara_print('reparsing', self.view.file_name())
        self.session_is_loaded = False
        if not self.session.reparse():
            clara_print('error during parsing of', self.view.file_name(), 
                'this session is disabled.')
            self.session = None
            return
        self.session_is_loaded = True
        clara_print('reparsed', self.view.file_name())
