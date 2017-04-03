from .create_auto_complete_triggers import ClaraCreateAutoCompleteTriggersCommand
from .diagnose import ClaraDiagnoseCommand
from .generate_system_headers import ClaraGenerateSystemHeadersCommand
from .insert_diagnosis import ClaraInsertDiagnosisCommand
from .open_settings import ClaraOpenSettingsCommand

__all__ = [
    'ClaraCreateAutoCompleteTriggersCommand', 
    'ClaraDiagnoseCommand', 
    'ClaraGenerateSystemHeadersCommand', 
    'ClaraInsertDiagnosisCommand', 
    'ClaraOpenSettingsCommand']

print('clara:', 'available commmands:')
for command in __all__:
    import re
    def convert(name):
        s1 = re.sub('(.)([A-Z][a-z]+)', r'\1_\2', name)
        cmd = re.sub('([a-z0-9])([A-Z])', r'\1_\2', s1).lower()
        if cmd.endswith('_command'):
            cmd = cmd[:-len('_command')]
        return cmd
    print('\t{}'.format(convert(command)))
