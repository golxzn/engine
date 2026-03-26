
add_library(gzn_deps INTERFACE)
add_library(gzn::deps ALIAS gzn_deps)

# >--------------------- | view backends |-----------------------< #

function(select_view_backend glad_extensions)
  if (MSVC)
    gzn_msg(STATUS "View backend: WinAPI")
    return()
  endif()

  find_package(wayland COMPONENTS client server cursor egl REQUIRED)

  if (WAYLAND_FOUND)
    target_link_libraries(gzn_deps INTERFACE
      wayland::client
      wayland::server
      wayland::cursor
      wayland::egl
    )

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
    endfunction()

    add_wayland_protocol_target(
      xdg_shell "/stable/xdg-shell/xdg-shell.xml" xdg-shell
    )
    set(glad_extensions VK_KHR_wayland_surface PARENT_SCOPE)
    gzn_msg(STATUS "View backend: Wayland")
    return()
  endif()

  find_package(X11)
  if (X11_FOUND)
    target_link_libraries(gzn_deps INTERFACE X11::X11 X11::xcb X11::Xkb)
    target_compile_definitions(gzn_deps INTERFACE GZN_VIEW_BACKEND_X11)
    set(glad_extensions "EGL_KHR_platform_x11;EGL_EXT_platform_xcb" PARENT_SCOPE)
    gzn_msg(STATUS "View backend: X11")
    return()
  endif()

  gzn_msg(FATAL_ERROR "Failed to find view backend!")

endfunction()

set(glad_apis)
set(glad_extensions)
select_view_backend(glad_extensions )

# >--------------------------| OpenGL |--------------------------< #

if(GZN_GFX_BACKEND MATCHES "^(any|opengl)$")
  find_package(OpenGL)
  target_link_libraries(gzn_deps INTERFACE OpenGL::EGL OpenGL::GLES3)

  list(APPEND glad_apis
    gl:core=4.1
    egl=1.3
    # glx for x11
    # wgl for wayland
  )
  list(APPEND glad_extensions
    EGL_EXT_device_base
    EGL_EXT_device_query
    EGL_EXT_platform_base
    EGL_EXT_platform_query
  )
  # OpenGL::OpenGL  maybe we don't need it thanks to GLAD
endif()

# >--------------------------| Vulkan |--------------------------< #

if(GZN_GFX_BACKEND MATCHES "^(any|vulkan)$")
  find_package(Vulkan)
  target_link_libraries(gzn_deps INTERFACE Vulkan::Vulkan)

  list(APPEND glad_apis vulkan=1.1)
  list(APPEND glad_extensions
    VK_EXT_debug_utils
    VK_KHR_swapchain
  )
endif()

# >---------------------------| GLAD |---------------------------< #

add_subdirectory(glad2/cmake)
glad_add_library(glad STATIC LOADER API ${glad_apis} EXTENSIONS ${glad_extension})
target_link_libraries(gzn_deps INTERFACE glad)

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

