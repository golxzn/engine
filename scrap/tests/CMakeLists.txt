include(FetchContent)

set(BUILD_SHARED_LIBS OFF)

FetchContent_Declare(Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v3.11.0
)
FetchContent_MakeAvailable(Catch2)
list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)

include(CTest)
include(Catch)

file(GLOB_RECURSE test_sources CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/test-*.*pp"
)

set(target gzn-engine-tests)
add_executable(${target} ${test_sources})

set_target_properties(${target} PROPERTIES
  FOLDER "gzn/tests"
  CXX_STANDARD ${GZN_CXX_STANDARD}
)
target_include_directories(${target} PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/../
)
target_link_libraries(${target} PRIVATE gzn::engine Catch2::Catch2WithMain)

catch_discover_tests(${target}
  WORKING_DIRECTORY ${gzn_root}/bin
)

