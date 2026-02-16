include(CMakeDependentOption)

# =-=-=-=-=-=-=-=-=-=-=-=-=-=-=    optinos    =-=-=-=-=-=-=-=-=-=-=-=-=-=-= #

macro(depopt)
  cmake_dependent_option(${ARGV})
endmacro()

option(GZN_DEV_MODE         "Developer mode"         OFF)
option(GZN_BUILD_DOCS       "Build documentations"   OFF)

depopt(GZN_BUILD_TESTS      "Build & Run tests"      ON GZN_DEV_MODE OFF)
depopt(GZN_BUILD_EXAMPLES   "Build examples"         ON GZN_DEV_MODE OFF)
depopt(GZN_BUILD_BENCHMARKS "Build & Run benchmarks" ON GZN_DEV_MODE OFF)
depopt(GZN_VERBOSE_LOGGING  "Enable verbose logging" ON GZN_DEV_MODE OFF)

option(GZN_DISABLE_ASSERTIONS "Forcely disable assertions in debug mode" OFF)


function(gzn_msg status)
  if (GZN_VERBOSE_LOGGING)
    list(POP_FRONT ARGV ARGV)
    message(${status} "[gzn] " ${ARGV})
  endif()
endfunction()

# =-=-=-=-=-=-=-=-=-=-=-=-=-=-= configuration =-=-=-=-=-=-=-=-=-=-=-=-=-=-= #

set(GZN_CXX_STANDARD 23 CACHE STRING "C++ Standard")
set_property(CACHE GZN_CXX_STANDARD PROPERTY STRINGS 23 26)

set(GZN_GFX_BACKEND any CACHE STRING "GFX backend")
set_property(CACHE GZN_GFX_BACKEND PROPERTY STRINGS
  any metal vulkan opengl opengl_es2
)

set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

if(MSVC)
  set(CMAKE_DEBUG_POSTFIX "d")
endif()

if(NOT (CMAKE_CONFIGURATION_TYPES OR CMAKE_BUILD_TYPE))
  gzn_msg(STATUS "Setting build type to 'Release' as none was specified.")
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY VALUE "Release")
endif()

include(OptimizeForArchitecture)
OptimizeForArchitecture()

