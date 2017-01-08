# Clara
Semantic C++ code completion for Sublime Text 3

## User Guide
Please [read the wiki](https://github.com/rwols/Clara/wiki).

## Build Notes for Ubuntu / OSX
* Make sure you have at least Python 3.3
* Make sure you have CMake.
* Clone LLVM and Clang with SVN.
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
