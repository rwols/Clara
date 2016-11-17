# Clara
Semantic C++ code completion for Sublime Text 3

# Build Notes for Ubuntu

* Clone LLVM and Clang with SVN.
* Build LLVM and Clang with RTTI and exceptions enabled.
** You can do this with ccmake.
* LLVM and Clang static libraries are built with -fPIC, so you need to build Boost.Python with that option enabled too.
* As of this writing, strive to use Python 3.3, as that is what Sublime Text 3 uses.
* It seems to work fine with Python 3.5.

# Build Notes for OSX

* As above, clone LLVM and Clang, build them with RTTI and exceptions enabled.
* Download Boost sources and build (only) Boost.Python with Python3 support and -fPIC compiler flag.
* As of this writing, you'll get a crash on startup relating to python threads and/or dydl loading.

# User Guide

Generate the correct header information: go to the command palette and run the command "Clara: Generate System Headers". This will create a Clara.sublime-settings file in your User preferences directory.
Clara works best when you have a CMake project and a sublime project. In your top-level CMakeLists.txt file, make sure you have the line "set(CMAKE_COMPILE_COMMANDS ON)" somewhere. Then in your *.sublime-project file, make sure you have at least the following entry:

	"clara":
	{
		"build_folder": "${project_path}/build"
	}

alongside "settings", "folders", "build_settings" and whatnot. This way, Clara will pick up the compile commands for each C++ file, and use that for correct project-wide autocompletion.
After this, you should be all set.
