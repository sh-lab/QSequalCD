cmake_minimum_required(VERSION 3.20)
project(QSequalCD LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(qscd_core
  src/core/Commands.cpp
  src/core/DeckFactory.cpp
  src/core/Invariants.cpp
  src/core/Queries.cpp
)

target_include_directories(qscd_core PUBLIC include)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
  target_compile_options(qscd_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

enable_testing()

add_executable(qscd_core_tests
  tests/core/TestMain.cpp
  tests/core/DeckFactoryTests.cpp
  tests/core/StartGameTests.cpp
  tests/core/StartRoundTests.cpp
  tests/core/RevealTests.cpp
  tests/core/HandUsageTests.cpp
  tests/core/ScoringTests.cpp
  tests/core/FinishRoundTests.cpp
  tests/core/ExtraRoundTests.cpp
  tests/core/ContinuationTests.cpp
  tests/core/InvariantTests.cpp
  tests/core/CommandImmutabilityTests.cpp
  tests/core/ProjectTests.cpp
)

target_link_libraries(qscd_core_tests PRIVATE qscd_core)
target_include_directories(qscd_core_tests PRIVATE tests)

add_test(NAME qscd_core_tests COMMAND qscd_core_tests)
