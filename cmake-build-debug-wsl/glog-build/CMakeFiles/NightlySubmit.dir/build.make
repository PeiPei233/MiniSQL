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

# Utility rule file for NightlySubmit.

# Include any custom commands dependencies for this target.
include glog-build/CMakeFiles/NightlySubmit.dir/compiler_depend.make

# Include the progress variables for this target.
include glog-build/CMakeFiles/NightlySubmit.dir/progress.make

glog-build/CMakeFiles/NightlySubmit:
	cd /home/stormyx/MiniSQL/cmake-build-debug-wsl/glog-build && /usr/bin/ctest -D NightlySubmit

NightlySubmit: glog-build/CMakeFiles/NightlySubmit
NightlySubmit: glog-build/CMakeFiles/NightlySubmit.dir/build.make
.PHONY : NightlySubmit

# Rule to build all files generated by this target.
glog-build/CMakeFiles/NightlySubmit.dir/build: NightlySubmit
.PHONY : glog-build/CMakeFiles/NightlySubmit.dir/build

glog-build/CMakeFiles/NightlySubmit.dir/clean:
	cd /home/stormyx/MiniSQL/cmake-build-debug-wsl/glog-build && $(CMAKE_COMMAND) -P CMakeFiles/NightlySubmit.dir/cmake_clean.cmake
.PHONY : glog-build/CMakeFiles/NightlySubmit.dir/clean

glog-build/CMakeFiles/NightlySubmit.dir/depend:
	cd /home/stormyx/MiniSQL/cmake-build-debug-wsl && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/stormyx/MiniSQL /home/stormyx/MiniSQL/thirdparty/glog /home/stormyx/MiniSQL/cmake-build-debug-wsl /home/stormyx/MiniSQL/cmake-build-debug-wsl/glog-build /home/stormyx/MiniSQL/cmake-build-debug-wsl/glog-build/CMakeFiles/NightlySubmit.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : glog-build/CMakeFiles/NightlySubmit.dir/depend
