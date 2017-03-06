# Clara
Semantic C++ code completion for Sublime Text 3

## User Guide
Please [read the wiki][1].

## Build Notes for Ubuntu / OSX
* Make sure you have *exactly* Python 3.3 (the patch number doesn't matter).
* Make sure you have at least version 3.0 of CMake.
* [Clone LLVM and Clang with SVN][2].
* `$ cd /path/to/llvm`
* `$ pushd tools/clang/tools`
* `$ git clone --recursive https://github.com/rwols/Clara.git`
* Edit the CMakeLists.txt file in the current directory by adding the line 
  `add_clang_subdirectory(Clara)` to it.
* `$ popd`
* `$ mkdir build`
* `$ cd build`
* `$ cmake .. -DLLVM_ENABLE_EH=ON -DLLVM_ENABLE_RTTI=ON -DCMAKE_BUILD_TYPE=Release`
* `$ make ClaraInstall -j8` 
* This should build and deploy a shared library named Clara.so, and all of its
  dependencies, to your sublime text packages folder.

[1]: https://github.com/rwols/Clara/wiki
[2]: https://clang.llvm.org/get_started.html
