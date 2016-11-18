install(FILES
	Clara.sublime-commands
	Clara.sublime-settings
	ClaraInsertDiagnosis.py
	CodeCompleteAtCursorCommand.py
	DiagnoseCommand.py
	DiagnosticConsumer.py
	EventListener.py
	GenerateSystemHeadersCommand.py
	Main.sublime-menu
	OpenClaraSettingsCommand.py
	ViewEventListener.py

	DESTINATION "${claraInstallFolder}")

add_custom_target(ClaraPlugin
	${CMAKE_COMMAND} 
	-DBUILD_TYPE=${CMAKE_BUILD_TYPE} 
	-P ${CMAKE_CURRENT_BINARY_DIR}/cmake_install.cmake)
add_dependencies(ClaraPlugin Clara)