#import os, sys
#CURRENT_DIR = os.path.dirname(os.path.abspath(__file__))
#if CURRENT_DIR not in sys.path: sys.path.append(CURRENT_DIR)
#import cpp as Clara

#class DiagnosticConsumer(Clara.DiagnosticConsumer):
#	"""Consumes and handles diagnostic messages."""
#	def __init__(self):
#		super(DiagnosticConsumer, self).__init__()
#
#	def handleDiagnostic(self, level, infos):
#		
#		for info in infos:
#			print(info)
#
#	def beginSourceFile(self):
#		print('beginning source file...')
#		
#	def endSourceFile(self):
#		print('end source file...')