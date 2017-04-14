from .diagnose import ClaraDiagnoseCommand
from .insert_diagnosis import ClaraInsertDiagnosisCommand
from .open_settings import ClaraOpenSettingsCommand
from .write_auto_complete_triggers import ClaraWriteAutoCompleteTriggersCommand
from .write_system_headers import ClaraWriteSystemHeadersCommand

__all__ = [
    'ClaraDiagnoseCommand', 
    'ClaraInsertDiagnosisCommand', 
    'ClaraOpenSettingsCommand',
    'ClaraWriteAutoCompleteTriggersCommand', 
    'ClaraWriteSystemHeadersCommand' ]

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
