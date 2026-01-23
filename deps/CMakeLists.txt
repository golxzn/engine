
add_library(gzn_deps INTERFACE)
add_library(gzn::deps ALIAS gzn_deps)

# >--------------------------| OpenGL |--------------------------< #

find_package(OpenGL)
target_link_libraries(gzn_deps INTERFACE
  OpenGL::EGL
  OpenGL::GLU
)

# >--------------------------| Vulkan |--------------------------< #

find_package(Vulkan)
target_link_libraries(gzn_deps INTERFACE
  Vulkan::Vulkan
)

# >-------------------------- | X11 |----------------------------< #

find_package(X11)
if (X11_FOUND)
  target_link_libraries(gzn_deps INTERFACE ${X11_LIBRARIES})
  target_include_directories(gzn_deps INTERFACE ${X11_INCLUDE_DIR})
  target_compile_definitions(gzn_deps INTERFACE GZN_VIEW_BACKEND_X11)
endif()

# >---------------------------| GLM |----------------------------< #


option(GLM_ENABLE_CXX_20 "Enable C++ 20" ON)
add_subdirectory(glm)
target_link_libraries(gzn_deps INTERFACE glm::glm)

# >-------------------------| mimalloc |-------------------------< #

option(MI_BUILD_SHARED "Build shared library" OFF)
option(MI_BUILD_OBJECT "Build object library" OFF)
option(MI_BUILD_TESTS  "Build test executables" OFF)

add_subdirectory(mimalloc)
target_link_libraries(gzn_deps INTERFACE mimalloc-static)

# >--------------------------------------------------------------< #

