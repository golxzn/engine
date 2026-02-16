# ---- Dependencies ----

find_package(Doxygen REQUIRED doxygen)
if (NOT TARGET Doxygen::doxygen)
	gzn_msg(WARNING "Doxygen not found.")
	return()
endif()

set(doxyfile ${CMAKE_BINARY_DIR}/docs/Doxyfile)
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in" "${doxyfile}"
	@ONLY NEWLINE_STYLE LF
)

add_custom_target(clear_docs
	COMMENT "Cleaning documentation"
	COMMAND "${CMAKE_COMMAND}" -E remove_directory
		"${CMAKE_CURRENT_SOURCE_DIR}/html"
		"${CMAKE_CURRENT_SOURCE_DIR}/xml"
)

set(DOXYGEN_STRIP_FROM_PATH OFF)
doxygen_add_docs(generate_docs ALL
	COMMENT "Generating documentation"
	WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
	CONFIG_FILE "${doxyfile}"
)

set_target_properties(clear_docs generate_docs PROPERTIES
	FOLDER "gzn/custom/docs"
)

add_dependencies(generate_docs clear_docs)

