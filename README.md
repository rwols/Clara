# Clara
Semantic C++ code completion for Sublime Text 3

# Build Notes for Ubuntu

* Make sure you have at least Python 3.3
* Make sure you have Boost.Python configured for at least Python 3.3
* Clone LLVM and Clang with SVN.
* `$ cd /path/to/llvm`
* `$ pushd tools/clang/tools`
* `$ git clone https://github.com/rwols/Clara.git`
* Edit the CMakeLists.txt file in the current directory by adding the line `add_clang_subdirectory(clara)` to it.
* `$ popd`
* `$ mkdir build`
* `$ cd build`
* `$ cmake .. -DLLVM_ENABLE_EH=ON -DLLVM_ENABLE_RTTI=ON`
* `$ make Clara` 
* This should build a shared library named Clara.so, and all of its dependencies, in the folder lib/
* To deploy Clara to Sublime Text's packages' folder, do
* `$ make ClaraInstall`

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
