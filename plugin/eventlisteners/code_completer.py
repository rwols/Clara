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
        if not self.view.file_name():
            return
        self.phantoms = []
        self.phantom_set = sublime.PhantomSet(self.view)
        self.phantom_set.update(self.phantoms)
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
        self.session.code_complete_async(self.view.id(), unsaved_buffer, row, 
            col, None)

    def on_post_save(self):
        if self.session_is_loaded:
            thread = threading.Thread(target=self._reparse)
            # basename = os.path.basename(self.view.file_name())
            # ProgressIndicator(thread, self.view.file_name(), 
            #   "Reparsing {}".format(basename), "Reparsed {}".format(basename))
            thread.start()

    def _init_session_on_another_thread(self):
        thread = threading.Thread(target=self._init_session)
        # basename = os.path.basename(self.view.file_name())
        # ProgressIndicator(thread, self.view.file_name(), 
        #     "Parsing {}".format(basename), "Parsed {}".format(basename))
        thread.start()

    def _init_session(self):
        if is_header_file(self.view.file_name()):
            clara_print(self.view.file_name(), 'is a header file, skipping.')
            return
        self.view.set_status('~clara', 'Parsing...')
        compdb = get_compilation_database_for_view(self.view)
        if not compdb:
            clara_print('no compilation database for', self.view.file_name())
            return
        options = SessionOptions()
        fsopts = FileSystemOptions()
        options.invocation, fsopts.working_dir = compdb.get(self.view.file_name())
        options.file_manager = FileManager(fsopts)
        
        options.builtin_headers = os.path.join(sublime.packages_path(), 'Clara', 'include')
        headers_dict = self._load_headers()
        if headers_dict:
            options.system_headers = headers_dict['system_headers']
            options.frameworks = headers_dict['system_frameworks']
        else:
            clara_print('no headers!')
            return
        clara_print('loading', self.view.file_name(),'with working directory', 
            options.working_dir, 'and compiler invocation', 
            str(options.invocation))
        options.diagnostic_callback = self._diagnostic_handler
        options.log_callback = clara_print
        options.code_complete_callback = self._completion_handler
        options.file_name = self.view.file_name()
        ast_dir = os.path.join(options.working_dir, ".clara")
        options.ast_file = os.path.join(ast_dir, 
            os.path.basename(self.view.file_name()) + ".ast")
        clara_print('The AST working file will be', options.ast_file)
        os.makedirs(ast_dir, exist_ok=True)
        settings = sublime.load_settings(g_CLARA_SETTINGS)

        options.code_complete_include_macros = settings.get(
            'include_macros', True)

        options.code_complete_include_code_patterns = settings.get(
            'include_code_patterns', True)

        options.code_complete_include_globals = settings.get(
            'include_globals', True)

        options.code_complete_include_brief_comments = settings.get(
            'include_brief_comments', True)

        try: # Things may go wrong here.
            self.session = Session(options)
            self.session_is_loaded = True # Success
            settings.add_on_change('include_macros', lambda: self.session.set_code_complete_include_macros(settings.get('include_macros')))
            settings.add_on_change('include_code_patterns', lambda: self.session.set_code_complete_include_code_patterns(settings.get('include_code_patterns')))
            settings.add_on_change('include_globals', lambda: self.session.set_code_complete_include_globals(settings.get('include_globals')))
            settings.add_on_change('include_brief_comments', lambda: self.session.set_code_complete_include_brief_comments(settings.get('include_brief_comments')))
            settings.add_on_change('include_optional_arguments', lambda: self.session.set_code_complete_include_optional_arguments(settings.get('include_optional_arguments')))
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
        self.view.erase_status('~clara')

    def _load_headers(self):
        username = getpass.getuser()
        hostname = socket.gethostname()
        key = '{}@{}'.format(username, hostname)
        settings = sublime.load_settings(g_CLARA_SETTINGS)
        headers = settings.get(key)
        if headers is not None:
            return headers
        elif not CodeCompleter.loaded_headers_atleast_once:
            CodeCompleter.loaded_headers_atleast_once = True
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

    def _completion_handler(self, view_id, row, col, completions):
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
            'api_completions_only': False,
            'next_completion_if_showing': False})

    def _diagnostic_handler(self, file_name, severity, row, column, message):
        if severity == 'begin':
            clara_print('clearing phantoms for', self.view.file_name())
            self.phantoms = []
            self.phantom_set.update(self.phantoms)
            return
        if file_name != self.view.file_name():
            return
        if severity != 'error' and severity != 'warning':
            return
        point = clang_rowcol_to_sublime_point(self.view, row, column)
        region = sublime.Region(point, point)
        message = replace_single_quotes_by_tag(message, 'b')
        message = '<body id="Clara"><div id="diagnostic" class="{}">{}</div></body>'.format(severity, message)
        phantom = sublime.Phantom(region, message, sublime.LAYOUT_BELOW)
        self.phantoms.append(phantom)
        self.phantom_set.update(self.phantoms)
        clara_print('updated phantom set for', self.view.file_name())

    def _reparse(self):
        self.session_is_loaded = False
        clara_print('reparsing', self.view.file_name())
        self.view.set_status('~clara', 'Reparsing...')
        if not self.session.reparse():
            self.session = None
            sublime.error_message('An error occurred during reparsing of this file for auto-completions. The auto-completion session will be disabled.')
            self.view.erase_status('~clara')
            return
        self.session_is_loaded = True
        clara_print('reparsed', self.view.file_name())
        self.view.erase_status('~clara')
