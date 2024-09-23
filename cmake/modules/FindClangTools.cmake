# From https://github.com/apache/arrow/blob/master/cpp/cmake_modules/FindClangTools.cmake
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# Tries to find the clang-tidy and clang-format modules
#
# Usage of this module as follows:
#
#  find_package(ClangTools)
#
# Variables used by this module, they can change the default behaviour and need
# to be set before calling find_package:
#
#  ClangToolsBin_HOME -
#   When set, this path is inspected instead of standard library binary locations
#   to find clang-tidy and clang-format
#
# This module defines
#  CLANG_TIDY_BIN, The  path to the clang tidy binary
#  CLANG_TIDY_FOUND, Whether clang tidy was found
#  CLANG_FORMAT_BIN, The path to the clang format binary
#  CLANG_TIDY_FOUND, Whether clang format was found

if (NOT CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  if(DEFINED $ENV{CLANG_VERSION})
    set(COMPILER_VERSION_MAJOR $ENV{CLANG_VERSION})
    message(STATUS "ClangTools -- Not Clang, using CLANG_VERSION: ${COMPILER_VERSION_MAJOR}")
  else()
    set(COMPILER_VERSION_MAJOR 13)
    message(STATUS "ClangTools -- Not Clang, using default: ${COMPILER_VERSION_MAJOR}")
  endif ()
else ()
  # ensure we use Clang Tools matching the compiler version when having multiple versions installed
  string(REPLACE "." ";" COMPILER_VERSION_LIST ${CMAKE_CXX_COMPILER_VERSION})
  list(GET COMPILER_VERSION_LIST 0 COMPILER_VERSION_MAJOR)
  # Debian packages usually have a major version suffix in /usr/bin, e.g. /usr/bin/clang-format-13
  message(STATUS "ClangTools -- Detected Major Version: ${COMPILER_VERSION_MAJOR}")
  # When CLANG_TOOLS_PATH is set, we can find it there e.g. /usr/lib/llvm-13/bin/clang-format
  message(STATUS "ClangTools -- Detected Compiler Path: $ENV{CLANG_TOOLS_PATH}")
endif ()

find_program(CLANG_TIDY_BIN
  NAMES clang-tidy-${COMPILER_VERSION_MAJOR} clang-tidy
  PATHS $ENV{CLANG_TOOLS_PATH}/bin usr/local/bin /usr/bin
  NO_DEFAULT_PATH
)

if ( "${CLANG_TIDY_BIN}" STREQUAL "CLANG_TIDY_BIN-NOTFOUND" )
  set(CLANG_TIDY_FOUND 0)
  message(STATUS "clang-tidy not found")
else()
  set(CLANG_TIDY_FOUND 1)
  message(STATUS "clang-tidy found at ${CLANG_TIDY_BIN}")
endif()

find_program(CLANG_FORMAT_BIN
  NAMES clang-format-${COMPILER_VERSION_MAJOR} clang-format
  PATHS $ENV{CLANG_TOOLS_PATH}/bin /usr/local/bin /usr/bin
  NO_DEFAULT_PATH
)

if ( "${CLANG_FORMAT_BIN}" STREQUAL "CLANG_FORMAT_BIN-NOTFOUND" )
  set(CLANG_FORMAT_FOUND 0)
  message(STATUS "clang-format not found")
else()
  set(CLANG_FORMAT_FOUND 1)
  message(STATUS "clang-format found at ${CLANG_FORMAT_BIN}")
endif()
