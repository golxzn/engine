include(CMakeDependentOption)

# =-=-=-=-=-=-=-=-=-=-=-=-=-=-=    optinos    =-=-=-=-=-=-=-=-=-=-=-=-=-=-= #

option(GZN_DEV_MODE         "Developer mode"         OFF)
option(GZN_BUILD_DOCS       "Build documentations"   OFF)
option(GZN_BUILD_TESTS      "Build & Run tests"      ${GZN_DEV_MODE})
option(GZN_BUILD_EXAMPLES   "Build examples"         ${GZN_DEV_MODE})
option(GZN_BUILD_BENCHMARKS "Build & Run benchmarks" ${GZN_DEV_MODE})

option(GZN_DISABLE_ASSERTIONS "Forcely disable assertions in debug mode" OFF)


# =-=-=-=-=-=-=-=-=-=-=-=-=-=-= configuration =-=-=-=-=-=-=-=-=-=-=-=-=-=-= #

set(GZN_CXX_STANDARD 23 CACHE STRING "C++ Standard")
set_property(CACHE GZN_CXX_STANDARD PROPERTY STRINGS 23 26)

set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

if(MSVC)
	set(CMAKE_DEBUG_POSTFIX "d")
endif()

if(NOT (CMAKE_CONFIGURATION_TYPES OR CMAKE_BUILD_TYPE))
	message(STATUS "Setting build type to 'Release' as none was specified.")
	set_property(CACHE CMAKE_BUILD_TYPE PROPERTY VALUE "Release")
endif()

include(OptimizeForArchitecture)
OptimizeForArchitecture()

