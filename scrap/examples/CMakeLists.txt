
set(current_dir ${CMAKE_CURRENT_SOURCE_DIR})
file(GLOB example_directories RELATIVE ${current_dir} ${current_dir}/*)

foreach(example IN LISTS example_directories)
  if(NOT IS_DIRECTORY ${current_dir}/${example})
    continue()
  endif()

  message(STATUS "Creating ${example} example")
  file(GLOB_RECURSE src CONFIGURE_DEPENDS ${current_dir}/${example}/*.*pp)
  add_executable(${example} ${src})
  target_link_libraries(${example} PUBLIC gzn::engine)
  set_target_properties(${example} PROPERTIES
    FOLDER "gzn/examples"
    CXX_STANDARD ${GZN_CXX_STANDARD}
    RUNTIME_OUTPUT_DIRECTORY ${gzn_root}/bin
  )
endforeach()

unset(current_dir)

