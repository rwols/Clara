from .Clara import *

class DiagnosticPrinter(DiagnosticConsumer):
	"""Consumes and handles diagnostic messages."""
	def __init__(self, printer):
		super(DiagnosticConsumer, self).__init__()
		self.printer = printer;

	def handleDiagnostic(self, level, infos):
		for info in infos:
			self.printer(info)

	def beginSourceFile(self):
		self.printer('beginning source file...')
		
	def endSourceFile(self):
		self.printer('end source file...')