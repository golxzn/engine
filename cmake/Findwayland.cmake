# Based on Martin Gräßlin's <mgraesslin@kde.org> work:
# https://github.com/LunarG/VulkanSamples/blob/master/cmake/FindWayland.cmake
#
# Findwayland.cmake
#
# Locate Wayland components on Unix-like systems and provide modern
# imported targets for target-based linking.
#
# Usage:
#
#   find_package(wayland COMPONENTS client server egl cursor REQUIRED)
#
# Imported Targets:
#
#   wayland::client
#   wayland::server
#   wayland::egl
#   wayland::cursor
#
# Each component provides:
#   - INTERFACE_INCLUDE_DIRECTORIES
#   - IMPORTED_LOCATION
#
# Result Variables:
#
#   wayland_FOUND                  - True if all requested components were found
#   wayland_<component>_FOUND      - True if specific component was found
#
# Component-specific cache variables:
#
#   WAYLAND_<component>_INCLUDE_DIR
#   WAYLAND_<component>_LIBRARY
#
# If no COMPONENTS are specified, the default component is:
#
#   client
#
# This module prefers pkg-config when available but falls back to
# find_path() and find_library().
#
# Copyright (c) 2026 Ruslan Golovinskii <golxzn@gmail.com>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.IF (NOT WIN32)


if (WIN32)
  set(wayland_FOUND FALSE)
  return()
endif()

include(FindPackageHandleStandardArgs)
find_package(PkgConfig QUIET)

if (NOT wayland_FIND_COMPONENTS)
  set(wayland_FIND_COMPONENTS client)
endif()

set(_wayland_required_vars)

foreach(component IN LISTS wayland_FIND_COMPONENTS)

  string(TOLOWER "${component}" component)

  set(_pkg "wayland-${component}")
  set(_header "wayland-${component}.h")
  set(_lib "wayland-${component}")

  if (PKG_CONFIG_FOUND)
    pkg_check_modules(PC_${component} QUIET ${_pkg})
  endif()

  find_path(WAYLAND_${component}_INCLUDE_DIR
    NAMES ${_header}
    HINTS ${PC_${component}_INCLUDE_DIRS}
  )

  find_library(WAYLAND_${component}_LIBRARY
    NAMES ${_lib}
    HINTS ${PC_${component}_LIBRARY_DIRS}
  )

  if (WAYLAND_${component}_INCLUDE_DIR AND WAYLAND_${component}_LIBRARY)
    add_library(wayland_${component} UNKNOWN IMPORTED)

    set_target_properties(wayland_${component} PROPERTIES
      IMPORTED_LOCATION "${WAYLAND_${component}_LIBRARY}"
      INTERFACE_INCLUDE_DIRECTORIES "${WAYLAND_${component}_INCLUDE_DIR}"
    )

    add_library(wayland::${component} ALIAS wayland_${component})

    set(wayland_${component}_FOUND TRUE)
    list(APPEND _wayland_required_vars WAYLAND_${component}_LIBRARY)

  else()
    set(wayland_${component}_FOUND FALSE)
  endif()
endforeach()

find_package_handle_standard_args(wayland
  REQUIRED_VARS _wayland_required_vars
  HANDLE_COMPONENTS
)

mark_as_advanced(${_wayland_required_vars})
