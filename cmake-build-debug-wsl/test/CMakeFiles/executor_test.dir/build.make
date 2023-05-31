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
include test/CMakeFiles/executor_test.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include test/CMakeFiles/executor_test.dir/compiler_depend.make

# Include the progress variables for this target.
include test/CMakeFiles/executor_test.dir/progress.make

# Include the compile flags for this target's objects.
include test/CMakeFiles/executor_test.dir/flags.make

test/CMakeFiles/executor_test.dir/execution/executor_test.cpp.o: test/CMakeFiles/executor_test.dir/flags.make
test/CMakeFiles/executor_test.dir/execution/executor_test.cpp.o: ../test/execution/executor_test.cpp
test/CMakeFiles/executor_test.dir/execution/executor_test.cpp.o: test/CMakeFiles/executor_test.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/stormyx/MiniSQL/cmake-build-debug-wsl/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object test/CMakeFiles/executor_test.dir/execution/executor_test.cpp.o"
	cd /home/stormyx/MiniSQL/cmake-build-debug-wsl/test && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT test/CMakeFiles/executor_test.dir/execution/executor_test.cpp.o -MF CMakeFiles/executor_test.dir/execution/executor_test.cpp.o.d -o CMakeFiles/executor_test.dir/execution/executor_test.cpp.o -c /home/stormyx/MiniSQL/test/execution/executor_test.cpp

test/CMakeFiles/executor_test.dir/execution/executor_test.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/executor_test.dir/execution/executor_test.cpp.i"
	cd /home/stormyx/MiniSQL/cmake-build-debug-wsl/test && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/stormyx/MiniSQL/test/execution/executor_test.cpp > CMakeFiles/executor_test.dir/execution/executor_test.cpp.i

test/CMakeFiles/executor_test.dir/execution/executor_test.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/executor_test.dir/execution/executor_test.cpp.s"
	cd /home/stormyx/MiniSQL/cmake-build-debug-wsl/test && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/stormyx/MiniSQL/test/execution/executor_test.cpp -o CMakeFiles/executor_test.dir/execution/executor_test.cpp.s

# Object files for target executor_test
executor_test_OBJECTS = \
"CMakeFiles/executor_test.dir/execution/executor_test.cpp.o"

# External object files for target executor_test
executor_test_EXTERNAL_OBJECTS =

test/executor_test: test/CMakeFiles/executor_test.dir/execution/executor_test.cpp.o
test/executor_test: test/CMakeFiles/executor_test.dir/build.make
test/executor_test: bin/libzSql.so
test/executor_test: test/libminisql_test_main.so
test/executor_test: glog-build/libglogd.so.0.6.0
test/executor_test: /usr/local/lib/libgflags.a
test/executor_test: lib/libgtestd.so.1.11.0
test/executor_test: test/CMakeFiles/executor_test.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/stormyx/MiniSQL/cmake-build-debug-wsl/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX executable executor_test"
	cd /home/stormyx/MiniSQL/cmake-build-debug-wsl/test && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/executor_test.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
test/CMakeFiles/executor_test.dir/build: test/executor_test
.PHONY : test/CMakeFiles/executor_test.dir/build

test/CMakeFiles/executor_test.dir/clean:
	cd /home/stormyx/MiniSQL/cmake-build-debug-wsl/test && $(CMAKE_COMMAND) -P CMakeFiles/executor_test.dir/cmake_clean.cmake
.PHONY : test/CMakeFiles/executor_test.dir/clean

test/CMakeFiles/executor_test.dir/depend:
	cd /home/stormyx/MiniSQL/cmake-build-debug-wsl && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/stormyx/MiniSQL /home/stormyx/MiniSQL/test /home/stormyx/MiniSQL/cmake-build-debug-wsl /home/stormyx/MiniSQL/cmake-build-debug-wsl/test /home/stormyx/MiniSQL/cmake-build-debug-wsl/test/CMakeFiles/executor_test.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : test/CMakeFiles/executor_test.dir/depend

