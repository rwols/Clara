import sublime, sublime_plugin, subprocess, os, getpass, socket

class ClaraWriteSystemHeadersCommand(sublime_plugin.ApplicationCommand):
    """Writes system header info."""

    def subprocess(self, cmd):
        if sublime.platform() == 'linux':
            shell_cmd = [os.getenv('SHELL'), '-c', cmd]
        elif sublime.platform() == 'osx':
            shell_cmd = [os.getenv('SHELL'), '-l', '-c', cmd]
        else: # windows
            shell_cmd = [os.getenv('SHELL'), '-c', cmd]
        proc = subprocess.Popen(
            shell_cmd,
            env=os.environ,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            shell=False,
            cwd=os.getenv('HOME'))
        outs, errs = proc.communicate()
        errs = errs.decode('utf-8')
        if errs:
            sublime.error_message(errs)
            return None
        return outs.decode('utf-8').strip()

    def run(self):
        
        SHELL_CMD1 = """clang++ -E -x c++ - -v < /dev/null 2>&1 | awk '/#include <...> search starts here/{f=1;next} /End of search list./{f=0} f'"""
        SHELL_CMD2 = """clang++ -E -x c++ - -v < /dev/null 2>&1 | awk 'BEGIN{FS="-resource-dir"} /-resource-dir/ {print $2}' | awk '{print $1;}'"""

        output = self.subprocess(SHELL_CMD1)
        raw_headers = output.splitlines()
        frameworks = []
        headers = []
        for raw_header in raw_headers:
            raw_header = raw_header.strip()
            if raw_header.endswith(' (framework directory)'):
                raw_header = raw_header[:-len(' (framework directory)')]
                frameworks.append(os.path.abspath(raw_header))
            else:
                headers.append(os.path.abspath(raw_header))
        output = self.subprocess(SHELL_CMD2)
        resource_dir = os.path.abspath(output)
        builtin_include = os.path.join(resource_dir, 'include')
        if builtin_include in headers: headers.remove(builtin_include)
        settings = sublime.load_settings('Clara.sublime-settings')
        username = getpass.getuser()
        hostname = socket.gethostname()
        key = '{}@{}'.format(username, hostname)
        header_dict = {
            'system_headers': headers,
            'system_frameworks': frameworks}
        settings.set(key, header_dict)
        sublime.save_settings('Clara.sublime-settings')
        if sublime.ok_cancel_dialog('System headers have been saved in user settings. Do you want to view them now?'):
            sublime.run_command('edit_settings', args={'base_file': '${packages}/Clara/Clara.sublime-settings'})

    def description(self):
        return 'Write System Headers...'
