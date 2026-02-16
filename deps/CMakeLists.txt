
add_library(gzn_deps INTERFACE)
add_library(gzn::deps ALIAS gzn_deps)

# >--------------------- | view backends |-----------------------< #

function(select_view_backend)
  if (MSVC)
    gzn_msg(STATUS "View backend: WinAPI")
    return()
  endif()

  find_package(wayland)
  if (WAYLAND_FOUND)

    # target_link_libraries(gzn_deps INTERFACE wayland)
    target_link_libraries(gzn_deps INTERFACE ${WAYLAND_LIBRARIES})
    target_include_directories(gzn_deps INTERFACE ${WAYLAND_INCLUDE_DIR})
    target_compile_definitions(gzn_deps INTERFACE GZN_VIEW_BACKEND_WAYLAND)

    function(add_wayland_protocol_target TARGET_NAME SPEC NAME)
      set(spec_path /usr/share/wayland-protocols${SPEC})
      set(out_path ${CMAKE_BINARY_DIR}/deps/${NAME})
      make_directory(${out_path}/include)
      make_directory(${out_path}/src)

      execute_process(
        COMMAND wayland-scanner client-header ${spec_path} ${out_path}/include/${NAME}.h
        COMMAND wayland-scanner private-code ${spec_path} ${out_path}/src/${NAME}.c
        OUTPUT_FILE ${out_path}/src/${NAME}.c
      )
      add_library(${TARGET_NAME} STATIC ${out_path}/src/${NAME}.c)
      target_include_directories(${TARGET_NAME} PUBLIC ${out_path}/include/)

      target_link_libraries(gzn_deps INTERFACE ${TARGET_NAME})
      # target_include_directories(gzn_deps INTERFACE ${out_path}/include)
    endfunction()

    add_wayland_protocol_target(xdg_shell "/stable/xdg-shell/xdg-shell.xml" xdg-shell)
    gzn_msg(STATUS "View backend: Wayland")
    return()
  endif()

  find_package(X11)
  if (X11_FOUND)
    target_link_libraries(gzn_deps INTERFACE X11::X11 X11::xcb X11::Xkb)
    target_compile_definitions(gzn_deps INTERFACE GZN_VIEW_BACKEND_X11)
    gzn_msg(STATUS "View backend: X11")
    return()
  endif()

  gzn_msg(FATAL_ERROR "Failed to find view backend!")

endfunction()

select_view_backend()

# >--------------------------| OpenGL |--------------------------< #

if(GZN_GFX_BACKEND MATCHES "^(any|opengl|opengl_es)$")
  find_package(OpenGL)
  target_link_libraries(gzn_deps INTERFACE OpenGL::EGL)

  if(GZN_GFX_BACKEND MATCHES "^(any|opengl)$")
    target_link_libraries(gzn_deps INTERFACE OpenGL::OpenGL)
  endif()

  if(GZN_GFX_BACKEND MATCHES "^(any|opengl_es)$")
    target_link_libraries(gzn_deps INTERFACE OpenGL::GLES3)
  endif()
endif()

# >--------------------------| Vulkan |--------------------------< #

if(GZN_GFX_BACKEND MATCHES "^(any|vulkan)$")
  find_package(Vulkan)
  target_link_libraries(gzn_deps INTERFACE Vulkan::Vulkan Vulkan::Headers)
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

