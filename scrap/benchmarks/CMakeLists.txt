include(FetchContent)

set(BUILD_SHARED_LIBS OFF)

FetchContent_Declare(nanobench
 GIT_REPOSITORY https://github.com/martinus/nanobench.git
 GIT_TAG v4.1.0
 GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(nanobench)

include(CTest)

file(GLOB_RECURSE bench_files CONFIGURE_DEPENDS
 "${CMAKE_CURRENT_SOURCE_DIR}/bench-*.*pp"
)

foreach(bench_file IN LISTS bench_files)
  get_filename_component(bench_name ${bench_file} NAME_WE)
  add_executable(${bench_name} ${bench_file})

  set_target_properties(${bench_name} PROPERTIES
    FOLDER "gzn/tests"
    CXX_STANDARD ${GZN_CXX_STANDARD}
  )
  target_include_directories(${bench_name} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../
  )
  target_link_libraries(${bench_name} PRIVATE gzn::engine nanobench)

  add_test(
  NAME ${bench_name}
  COMMAND ${bench_name}
  WORKING_DIRECTORY ${gzn_root}/bin
 )
endforeach()


