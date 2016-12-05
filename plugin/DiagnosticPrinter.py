from .Clara import *

class DiagnosticPrinter(DiagnosticConsumer):
	"""Consumes and handles diagnostic messages."""
	def __init__(self, printer):
		super(DiagnosticConsumer, self).__init__()
		if not callable(printer): raise TypeError
		self.printer = printer;
		self.printer('Initialized DiagnosticPrinter.')

	def handleNote(filename, row, column, message):
		self.printer('NOTE: ' + filename + ':' + str(row) + ':' + str(column) + ': ' + message)

	def handleRemark(filename, row, column, message):
		self.printer('REMARK: ' + filename + ':' + str(row) + ':' + str(column) + ': ' + message)

	def handleWarning(filename, row, column, message):
		self.printer('WARNING: ' + filename + ':' + str(row) + ':' + str(column) + ': ' + message)

	def handleError(filename, row, column, message):
		self.printer('ERROR: ' + filename + ':' + str(row) + ':' + str(column) + ': ' + message)

	def handleFatalError(filename, row, column, message):
		self.printer('FATAL ERROR: ' + filename + ':' + str(row) + ':' + str(column) + ': ' + message)