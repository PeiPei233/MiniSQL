# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.22

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:

#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:

# Disable VCS-based implicit rules.
% : %,v

# Disable VCS-based implicit rules.
% : RCS/%

# Disable VCS-based implicit rules.
% : RCS/%,v

# Disable VCS-based implicit rules.
% : SCCS/s.%

# Disable VCS-based implicit rules.
% : s.%

.SUFFIXES: .hpux_make_needs_suffix_list

# Command-line flag to silence nested $(MAKE).
$(VERBOSE)MAKESILENT = -s

#Suppress display of executed commands.
$(VERBOSE).SILENT:

# A target that is always out of date.
cmake_force:
.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/stormyx/MiniSQL

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/stormyx/MiniSQL/cmake-build-debug-wsl

# Include any dependencies generated for this target.
include glog-build/CMakeFiles/demangle_unittest.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include glog-build/CMakeFiles/demangle_unittest.dir/compiler_depend.make

# Include the progress variables for this target.
include glog-build/CMakeFiles/demangle_unittest.dir/progress.make

# Include the compile flags for this target's objects.
include glog-build/CMakeFiles/demangle_unittest.dir/flags.make

glog-build/CMakeFiles/demangle_unittest.dir/src/demangle_unittest.cc.o: glog-build/CMakeFiles/demangle_unittest.dir/flags.make
glog-build/CMakeFiles/demangle_unittest.dir/src/demangle_unittest.cc.o: ../thirdparty/glog/src/demangle_unittest.cc
glog-build/CMakeFiles/demangle_unittest.dir/src/demangle_unittest.cc.o: glog-build/CMakeFiles/demangle_unittest.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/stormyx/MiniSQL/cmake-build-debug-wsl/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object glog-build/CMakeFiles/demangle_unittest.dir/src/demangle_unittest.cc.o"
	cd /home/stormyx/MiniSQL/cmake-build-debug-wsl/glog-build && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT glog-build/CMakeFiles/demangle_unittest.dir/src/demangle_unittest.cc.o -MF CMakeFiles/demangle_unittest.dir/src/demangle_unittest.cc.o.d -o CMakeFiles/demangle_unittest.dir/src/demangle_unittest.cc.o -c /home/stormyx/MiniSQL/thirdparty/glog/src/demangle_unittest.cc

glog-build/CMakeFiles/demangle_unittest.dir/src/demangle_unittest.cc.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/demangle_unittest.dir/src/demangle_unittest.cc.i"
	cd /home/stormyx/MiniSQL/cmake-build-debug-wsl/glog-build && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/stormyx/MiniSQL/thirdparty/glog/src/demangle_unittest.cc > CMakeFiles/demangle_unittest.dir/src/demangle_unittest.cc.i

glog-build/CMakeFiles/demangle_unittest.dir/src/demangle_unittest.cc.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/demangle_unittest.dir/src/demangle_unittest.cc.s"
	cd /home/stormyx/MiniSQL/cmake-build-debug-wsl/glog-build && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/stormyx/MiniSQL/thirdparty/glog/src/demangle_unittest.cc -o CMakeFiles/demangle_unittest.dir/src/demangle_unittest.cc.s

# Object files for target demangle_unittest
demangle_unittest_OBJECTS = \
"CMakeFiles/demangle_unittest.dir/src/demangle_unittest.cc.o"

# External object files for target demangle_unittest
demangle_unittest_EXTERNAL_OBJECTS =

glog-build/demangle_unittest: glog-build/CMakeFiles/demangle_unittest.dir/src/demangle_unittest.cc.o
glog-build/demangle_unittest: glog-build/CMakeFiles/demangle_unittest.dir/build.make
glog-build/demangle_unittest: glog-build/libglogtestd.a
glog-build/demangle_unittest: /usr/local/lib/libgflags.a
glog-build/demangle_unittest: glog-build/CMakeFiles/demangle_unittest.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/stormyx/MiniSQL/cmake-build-debug-wsl/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX executable demangle_unittest"
	cd /home/stormyx/MiniSQL/cmake-build-debug-wsl/glog-build && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/demangle_unittest.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
glog-build/CMakeFiles/demangle_unittest.dir/build: glog-build/demangle_unittest
.PHONY : glog-build/CMakeFiles/demangle_unittest.dir/build

glog-build/CMakeFiles/demangle_unittest.dir/clean:
	cd /home/stormyx/MiniSQL/cmake-build-debug-wsl/glog-build && $(CMAKE_COMMAND) -P CMakeFiles/demangle_unittest.dir/cmake_clean.cmake
.PHONY : glog-build/CMakeFiles/demangle_unittest.dir/clean

glog-build/CMakeFiles/demangle_unittest.dir/depend:
	cd /home/stormyx/MiniSQL/cmake-build-debug-wsl && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/stormyx/MiniSQL /home/stormyx/MiniSQL/thirdparty/glog /home/stormyx/MiniSQL/cmake-build-debug-wsl /home/stormyx/MiniSQL/cmake-build-debug-wsl/glog-build /home/stormyx/MiniSQL/cmake-build-debug-wsl/glog-build/CMakeFiles/demangle_unittest.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : glog-build/CMakeFiles/demangle_unittest.dir/depend

