# Based on Martin Gräßlin's <mgraesslin@kde.org> work:
# https://github.com/LunarG/VulkanSamples/blob/master/cmake/FindWayland.cmake
# Try to find Wayland on a Unix system
#
# This will define:
#
#   wayland_FOUND       - True if wayland is found
#   wayland_LIBRARIES   - Link these to use wayland
#   wayland_INCLUDE_DIR - Include directory for wayland
#   wayland_DEFINITIONS - Compiler flags for using wayland
#
# In addition the following more fine grained variables will be defined:
#
#   wayland_CLIENT_FOUND  wayland_CLIENT_INCLUDE_DIR  wayland_CLIENT_LIBRARIES
#   wayland_SERVER_FOUND  wayland_SERVER_INCLUDE_DIR  wayland_SERVER_LIBRARIES
#   wayland_EGL_FOUND     wayland_EGL_INCLUDE_DIR     wayland_EGL_LIBRARIES
#
# Copyright (c) 2026 Ruslan Golovinskii <golxzn@gmail.com>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

IF (NOT WIN32)
  IF (WAYLAND_INCLUDE_DIR AND WAYLAND_LIBRARIES)
    # In the cache already
    SET(WAYLAND_FIND_QUIETLY TRUE)
  ENDIF ()

  # Use pkg-config to get the directories and then use these values
  # in the FIND_PATH() and FIND_LIBRARY() calls
  FIND_PACKAGE(PkgConfig)
  PKG_CHECK_MODULES(PKG_WAYLAND QUIET wayland-client wayland-server wayland-egl wayland-cursor)

  SET(WAYLAND_DEFINITIONS ${PKG_WAYLAND_CFLAGS})

  FIND_PATH(WAYLAND_CLIENT_INCLUDE_DIR  NAMES wayland-client.h HINTS ${PKG_WAYLAND_INCLUDE_DIRS})
  FIND_PATH(WAYLAND_SERVER_INCLUDE_DIR  NAMES wayland-server.h HINTS ${PKG_WAYLAND_INCLUDE_DIRS})
  FIND_PATH(WAYLAND_EGL_INCLUDE_DIR     NAMES wayland-egl.h    HINTS ${PKG_WAYLAND_INCLUDE_DIRS})
  FIND_PATH(WAYLAND_CURSOR_INCLUDE_DIR  NAMES wayland-cursor.h HINTS ${PKG_WAYLAND_INCLUDE_DIRS})

  FIND_LIBRARY(WAYLAND_CLIENT_LIBRARIES NAMES wayland-client   HINTS ${PKG_WAYLAND_LIBRARY_DIRS})
  FIND_LIBRARY(WAYLAND_SERVER_LIBRARIES NAMES wayland-server   HINTS ${PKG_WAYLAND_LIBRARY_DIRS})
  FIND_LIBRARY(WAYLAND_EGL_LIBRARIES    NAMES wayland-egl      HINTS ${PKG_WAYLAND_LIBRARY_DIRS})
  FIND_LIBRARY(WAYLAND_CURSOR_LIBRARIES NAMES wayland-cursor   HINTS ${PKG_WAYLAND_LIBRARY_DIRS})

  set(WAYLAND_INCLUDE_DIR ${WAYLAND_CLIENT_INCLUDE_DIR} ${WAYLAND_SERVER_INCLUDE_DIR} ${WAYLAND_EGL_INCLUDE_DIR} ${WAYLAND_CURSOR_INCLUDE_DIR})

  set(WAYLAND_LIBRARIES ${WAYLAND_CLIENT_LIBRARIES} ${WAYLAND_SERVER_LIBRARIES} ${WAYLAND_EGL_LIBRARIES} ${WAYLAND_CURSOR_LIBRARIES})

  list(REMOVE_DUPLICATES WAYLAND_INCLUDE_DIR)

  include(FindPackageHandleStandardArgs)

  FIND_PACKAGE_HANDLE_STANDARD_ARGS(WAYLAND_CLIENT  DEFAULT_MSG  WAYLAND_CLIENT_LIBRARIES  WAYLAND_CLIENT_INCLUDE_DIR)
  FIND_PACKAGE_HANDLE_STANDARD_ARGS(WAYLAND_SERVER  DEFAULT_MSG  WAYLAND_SERVER_LIBRARIES  WAYLAND_SERVER_INCLUDE_DIR)
  FIND_PACKAGE_HANDLE_STANDARD_ARGS(WAYLAND_EGL     DEFAULT_MSG  WAYLAND_EGL_LIBRARIES     WAYLAND_EGL_INCLUDE_DIR)
  FIND_PACKAGE_HANDLE_STANDARD_ARGS(WAYLAND_CURSOR  DEFAULT_MSG  WAYLAND_CURSOR_LIBRARIES  WAYLAND_CURSOR_INCLUDE_DIR)
  FIND_PACKAGE_HANDLE_STANDARD_ARGS(WAYLAND         DEFAULT_MSG  WAYLAND_LIBRARIES         WAYLAND_INCLUDE_DIR)

  MARK_AS_ADVANCED(
        WAYLAND_INCLUDE_DIR         WAYLAND_LIBRARIES
        WAYLAND_CLIENT_INCLUDE_DIR  WAYLAND_CLIENT_LIBRARIES
        WAYLAND_SERVER_INCLUDE_DIR  WAYLAND_SERVER_LIBRARIES
        WAYLAND_EGL_INCLUDE_DIR     WAYLAND_EGL_LIBRARIES
        WAYLAND_CURSOR_INCLUDE_DIR  WAYLAND_CURSOR_LIBRARIES
  )

ENDIF ()

# if(NOT WIN32)
#   if(wayland_INCLUDE_DIR AND wayland_LIBRARIES)
#     # In the cache already
#     set(wayland_FIND_QUIETLY TRUE)
#   endif()
#   find_package(PkgConfig)
#   include(FindPackageHandleStandardArgs)
#
#   set(wayland_COMPONENTS client server egl cursor)
#   set(wayland_DEFINITIONS ${PKG_wayland_CFLAGS})
#   PKG_CHECK_MODULES(PKG_WAYLAND QUIET
#     wayland-client wayland-server wayland-egl wayland-cursor
#   )
#
#   add_library(wayland INTERFACE)
#   foreach(component IN LISTS wayland_COMPONENTS)
#     set(name wayland-${component})
#
#     find_path(wayland_${component}_INCLUDE_DIR
#       NAMES wayland-${component}.h HINTS ${PKG_wayland_INCLUDE_DIRS}
#     )
#     find_path(wayland_${component}_LIB_DIR
#       NAMES wayland-${component}   HINTS ${PKG_wayland_LIBRARY_DIRS}
#     )
#     find_library(wayland_${component}_LIBRARIES
#       NAMES wayland-${component}   HINTS ${PKG_wayland_LIBRARY_DIRS}
#     )
#
#     add_library(wayland_${component} SHARED IMPORTED
#       ${wayland_${component}_LIBRARIES}
#     )
#     set_target_properties(wayland_${component} PROPERTIES
#       IMPORTED_LOCATION ${wayland_${component}_LIB_DIR}
#     )
#     target_include_directories(wayland_${component} INTERFACE
#       ${wayland_${component}_INCLUDE_DIR}
#     )
#     add_library(wayland::${component} ALIAS wayland_${component})
#
#     # wayland interface
#     target_link_libraries(wayland INTERFACE wayland::${component})
#     list(APPEND wayland_LIBRARIES ${wayland_${component}_LIBRARIES})
#     list(APPEND wayland_INCLUDE_DIR ${wayland_${component}_INCLUDE_DIR})
#   endforeach()
#
#   list(REMOVE_DUPLICATES wayland_LIBRARIES)
#   list(REMOVE_DUPLICATES wayland_INCLUDE_DIR)
#   find_package_handle_standard_args(wayland DEFAULT_MSG
#     wayland_LIBRARIES
#     wayland_INCLUDE_DIR
#   )
#   mark_as_advanced(wayland_LIBRARIES wayland_INCLUDE_DIR)
# endif ()
