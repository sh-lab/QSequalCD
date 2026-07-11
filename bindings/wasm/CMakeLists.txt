cmake_minimum_required(VERSION 3.20)
project(qscd_wasm LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

get_filename_component(QSCD_ROOT "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)

add_library(qscd_core STATIC
  "${QSCD_ROOT}/src/core/Commands.cpp"
  "${QSCD_ROOT}/src/core/DeckFactory.cpp"
  "${QSCD_ROOT}/src/core/Invariants.cpp"
  "${QSCD_ROOT}/src/core/Queries.cpp"
)

target_include_directories(qscd_core PUBLIC "${QSCD_ROOT}/include")

if(EMSCRIPTEN)
  add_executable(qscd_wasm qscd_wasm.cpp)
else()
  add_library(qscd_wasm STATIC qscd_wasm.cpp)
endif()
target_include_directories(qscd_wasm PRIVATE "${QSCD_ROOT}/include" "${CMAKE_CURRENT_LIST_DIR}")
target_link_libraries(qscd_wasm PRIVATE qscd_core)

if(EMSCRIPTEN)
  set_target_properties(qscd_wasm PROPERTIES
    OUTPUT_NAME "qscd_wasm"
    SUFFIX ".js"
  )

  target_link_options(qscd_wasm PRIVATE
    "SHELL:-sMODULARIZE=1"
    "SHELL:-sEXPORT_ES6=1"
    "SHELL:-sEXPORT_NAME=createQSCDWasmModule"
    "SHELL:-sENVIRONMENT=web"
    "SHELL:-sALLOW_MEMORY_GROWTH=1"
    "SHELL:-sNO_EXIT_RUNTIME=1"
    "SHELL:-sEXPORTED_FUNCTIONS=['_createGame','_destroyGame','_startRound','_planHandUsage','_revealCards','_applyNullify','_finishRound','_requestExtraRound','_finishGame','_drawContinuationCards','_startContinuationGame','_getStateSnapshot','_malloc','_free']"
    "SHELL:-sEXPORTED_RUNTIME_METHODS=['cwrap','ccall','UTF8ToString','HEAPU32','HEAP32']"
    "SHELL:--no-entry"
  )
endif()
