cmake_minimum_required(VERSION 3.25)

set(gzn_root ${CMAKE_CURRENT_SOURCE_DIR})
list(APPEND CMAKE_MODULE_PATH
	${gzn_root}/cmake
)

project(gzn-engine
	VERSION      0.1.0
	DESCRIPTION  "some kinda engine"
	HOMEPAGE_URL https://golxzn/projects/gzn-engine.html
  LANGUAGES    C CXX
)

include(GNUInstallDirs)
include(GznOptions)

add_subdirectory(${gzn_root}/deps)
add_subdirectory(${gzn_root}/code)

include(${gzn_root}/scrap/scrap.cmake)

if(GZN_BUILD_DOCS)
	add_subdirectory(${gzn_root}/docs)
endif()

