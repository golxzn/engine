
set(lib_target ${PROJECT_NAME})

file(GLOB_RECURSE gzn_header_files CONFIGURE_DEPENDS
  ${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp
)
file(GLOB_RECURSE gzn_source_files CONFIGURE_DEPENDS
  ${CMAKE_CURRENT_SOURCE_DIR}/sources/*.cpp
)

add_library(${lib_target} ${gzn_header_files} ${gzn_source_files})
add_library(gzn::engine ALIAS ${lib_target})

if (MSVC)
  target_compile_options(${lib_target} PRIVATE /W4)
else()
  target_compile_options(${lib_target} PRIVATE -Wall -Wextra -Wpedantic)
endif()

target_include_directories(${lib_target} PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:$<INSTALL_PREFIX>/${CMAKE_INSTALL_INCLUDEDIR}>
)

include(GznGetAssemblyDialect)
gzn_get_assembly_dialect(asm_dialect)

target_compile_definitions(${lib_target} PUBLIC
  "GZN_$<UPPER_CASE:$<CONFIG>>"
  "GZN_COMPILER_$<UPPER_CASE:$<CXX_COMPILER_ID>>"
  "GZN_PLATFORM_$<UPPER_CASE:$<PLATFORM_ID>>"
  "GZN_PROCESSOR_$<UPPER_CASE:${CMAKE_SYSTEM_PROCESSOR}>"
  "GZN_ASM_DIALECT_${asm_dialect}"
)
if(NOT GZN_DISABLE_ASSERTIONS)
  target_compile_definitions(${lib_target} PUBLIC
  "$<$<CONFIG:Debug>:GZN_ASSERTIONS>"
)
endif()

target_link_libraries(${lib_target} PUBLIC gzn::deps)

set_target_properties(${lib_target} PROPERTIES
  FOLDER "gzn"
  VERSION        ${PROJECT_VERSION}
  CXX_STANDARD   ${GZN_CXX_STANDARD}
  PUBLIC_HEADER "${gzn_header_files}"
)

set(public_header_dir ${CMAKE_INSTALL_INCLUDEDIR}/${lib_target})
install(TARGETS ${lib_target} EXPORT ${PROJECT_NAME}
  LIBRARY       DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT shlib
  ARCHIVE       DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT lib
  RUNTIME       DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT bin
  PUBLIC_HEADER DESTINATION ${public_header_dir}    COMPONENT dev
)

unset(lib_target)

