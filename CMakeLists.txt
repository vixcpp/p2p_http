# ====================================================================
# Vix.cpp — P2P HTTP Adapter Module
# ====================================================================
# Purpose:
#   Provide HTTP routes that expose/bridge P2P runtime capabilities
#   through the Vix core Router without creating circular dependencies.
#
# Requires:
#   - vix::core (Router + HTTP types)
#   - vix::p2p  (P2PRuntime)
#
# Optional:
#   - vix::middleware (AUTO|ON|OFF) for auth hook / route policies
#
# Public Targets:
#   - vix_p2p_http   : STATIC or INTERFACE (header-only)
#   - vix::p2p_http  : namespaced alias
#
# Installation/Export:
#   Installs into umbrella export-set `VixTargets`.
# ====================================================================
cmake_minimum_required(VERSION 3.20)
project(vix_p2p_http VERSION 0.1.0 LANGUAGES CXX)

include(GNUInstallDirs)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# ====================================================================
# Options
# ====================================================================
# Standalone mode: allow pulling deps from sibling modules (../core, ../p2p, ../middleware)
option(VIX_P2P_HTTP_FETCH_DEPS "Allow adding sibling deps when building module standalone" ON)

# Optional middleware policy
set(VIX_P2P_HTTP_WITH_MIDDLEWARE "AUTO" CACHE STRING "Link middleware module (AUTO|ON|OFF)")
set_property(CACHE VIX_P2P_HTTP_WITH_MIDDLEWARE PROPERTY STRINGS AUTO ON OFF)

# ====================================================================
# Helpers
# ====================================================================
# Link/include/defs in a way that works for both STATIC and INTERFACE libs.
function(vix_p2p_http_apply_link onto scope)
  # Usage: vix_p2p_http_apply_link(<target> <PUBLIC|PRIVATE|INTERFACE> <item...>)
  # For INTERFACE libraries, CMake requires INTERFACE keyword (PUBLIC/PRIVATE are invalid).
  set(_t "${onto}")
  set(_scope "${scope}")
  set(_items ${ARGN})

  if (NOT TARGET "${_t}")
    message(FATAL_ERROR "[p2p_http] apply_link: target '${_t}' not found.")
  endif()

  get_target_property(_type "${_t}" TYPE)
  if (_type STREQUAL "INTERFACE_LIBRARY")
    target_link_libraries(${_t} INTERFACE ${_items})
  else()
    target_link_libraries(${_t} ${_scope} ${_items})
  endif()
endfunction()

function(vix_p2p_http_apply_defs onto scope)
  # Usage: vix_p2p_http_apply_defs(<target> <PUBLIC|PRIVATE|INTERFACE> <DEF...>)
  set(_t "${onto}")
  set(_scope "${scope}")
  set(_defs ${ARGN})

  if (NOT TARGET "${_t}")
    message(FATAL_ERROR "[p2p_http] apply_defs: target '${_t}' not found.")
  endif()

  get_target_property(_type "${_t}" TYPE)
  if (_type STREQUAL "INTERFACE_LIBRARY")
    target_compile_definitions(${_t} INTERFACE ${_defs})
  else()
    target_compile_definitions(${_t} ${_scope} ${_defs})
  endif()
endfunction()

function(vix_p2p_http_apply_includes onto scope)
  # Usage: vix_p2p_http_apply_includes(<target> <PUBLIC|PRIVATE|INTERFACE> <dir...>)
  set(_t "${onto}")
  set(_scope "${scope}")
  set(_dirs ${ARGN})

  if (NOT TARGET "${_t}")
    message(FATAL_ERROR "[p2p_http] apply_includes: target '${_t}' not found.")
  endif()

  get_target_property(_type "${_t}" TYPE)
  if (_type STREQUAL "INTERFACE_LIBRARY")
    target_include_directories(${_t} INTERFACE ${_dirs})
  else()
    target_include_directories(${_t} ${_scope} ${_dirs})
  endif()
endfunction()

# Normalize dependency presence in standalone builds (sibling add_subdirectory allowed).
function(vix_p2p_http_ensure_target _target _relpath _builddir _what)
  if (TARGET ${_target})
    return()
  endif()

  # Umbrella build must not try to add deps.
  if (DEFINED VIX_UMBRELLA_BUILD AND VIX_UMBRELLA_BUILD)
    message(FATAL_ERROR "[p2p_http] Umbrella build: ${_what} target '${_target}' must exist before p2p_http.")
  endif()

  if (NOT VIX_P2P_HTTP_FETCH_DEPS)
    message(FATAL_ERROR "[p2p_http] Standalone build: missing ${_what} target '${_target}'. Enable VIX_P2P_HTTP_FETCH_DEPS=ON or build from umbrella.")
  endif()

  if (EXISTS "${CMAKE_CURRENT_LIST_DIR}/${_relpath}/CMakeLists.txt")
    message(STATUS "[p2p_http] Adding ${_what} from sibling: ${_relpath}")
    add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/${_relpath}" "${_builddir}")
  else()
    message(FATAL_ERROR "[p2p_http] Missing ${_what} sources at '${_relpath}'.")
  endif()

  if (NOT TARGET ${_target})
    message(FATAL_ERROR "[p2p_http] ${_what} added but target '${_target}' still not found. Check that module exports this target name.")
  endif()
endfunction()

function(vix_p2p_http_try_link_middleware onto scope)
  if (VIX_P2P_HTTP_WITH_MIDDLEWARE STREQUAL "OFF")
    message(STATUS "[p2p_http] Middleware disabled (OFF)")
    vix_p2p_http_apply_defs(${onto} ${scope} VIX_P2P_HTTP_NO_MIDDLEWARE=1)
    return()
  endif()

  # If missing and we're standalone, try to add it.
  if (NOT TARGET vix::middleware
      AND NOT (DEFINED VIX_UMBRELLA_BUILD AND VIX_UMBRELLA_BUILD)
      AND VIX_P2P_HTTP_FETCH_DEPS)
    if (EXISTS "${CMAKE_CURRENT_LIST_DIR}/../middleware/CMakeLists.txt")
      message(STATUS "[p2p_http] Adding middleware from sibling: ../middleware")
      add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/../middleware" "middleware_build")
    endif()
  endif()

  if (TARGET vix::middleware)
    message(STATUS "[p2p_http] Middleware backend: vix::middleware")
    vix_p2p_http_apply_link(${onto} ${scope} vix::middleware)
    vix_p2p_http_apply_defs(${onto} ${scope} VIX_P2P_HTTP_WITH_MIDDLEWARE=1)
    return()
  endif()

  if (VIX_P2P_HTTP_WITH_MIDDLEWARE STREQUAL "ON")
    message(FATAL_ERROR "[p2p_http] Middleware required (ON) but vix::middleware not found.")
  endif()

  message(STATUS "[p2p_http] Middleware not detected; building without middleware. (AUTO)")
  vix_p2p_http_apply_defs(${onto} ${scope} VIX_P2P_HTTP_NO_MIDDLEWARE=1)
endfunction()

# ====================================================================
# Ensure required deps exist:
#   - vix::core
#   - vix::p2p
# ====================================================================
vix_p2p_http_ensure_target(vix::core ../core core_build "core")
vix_p2p_http_ensure_target(vix::p2p  ../p2p  p2p_build  "p2p")

# ====================================================================
# Sources
# ====================================================================
file(GLOB_RECURSE P2P_HTTP_SOURCES CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

# ====================================================================
# Library
# ====================================================================
set(_VIX_P2P_HTTP_MODE "HEADER-ONLY")
if (P2P_HTTP_SOURCES)
  message(STATUS "[p2p_http] Building STATIC library with detected sources.")
  add_library(vix_p2p_http STATIC ${P2P_HTTP_SOURCES})
  set(_VIX_P2P_HTTP_MODE "STATIC")
else()
  message(STATUS "[p2p_http] Building HEADER-ONLY library (no sources).")
  add_library(vix_p2p_http INTERFACE)
endif()

add_library(vix::p2p_http ALIAS vix_p2p_http)

# Includes (work for both STATIC + INTERFACE)
vix_p2p_http_apply_includes(vix_p2p_http PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

# Standard
target_compile_features(vix_p2p_http PUBLIC cxx_std_20)

# Warnings if umbrella provides it
if (TARGET vix_warnings)
  vix_p2p_http_apply_link(vix_p2p_http PUBLIC vix_warnings)
endif()

# Required deps
vix_p2p_http_apply_link(vix_p2p_http PUBLIC vix::core vix::p2p)

# Optional middleware
vix_p2p_http_try_link_middleware(vix_p2p_http PUBLIC)

# Sanitizers if umbrella provides them
if (VIX_ENABLE_SANITIZERS AND TARGET vix_sanitizers)
  vix_p2p_http_apply_link(vix_p2p_http PUBLIC vix_sanitizers)
endif()

# Properties (only for real library)
if (_VIX_P2P_HTTP_MODE STREQUAL "STATIC")
  set_target_properties(vix_p2p_http PROPERTIES
    OUTPUT_NAME vix_p2p_http
    VERSION ${PROJECT_VERSION}
    SOVERSION 0
    EXPORT_NAME p2p_http
  )
endif()

# ====================================================================
# Install / Export
# ====================================================================
# Always install headers
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  FILES_MATCHING PATTERN "*.hpp" PATTERN "*.h"
)

# Install target (works for STATIC and INTERFACE)
install(TARGETS vix_p2p_http
  EXPORT VixTargets
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# ====================================================================
# Tests
# ====================================================================
option(VIX_P2P_HTTP_BUILD_TESTS "Build p2p_http tests" OFF)
if (VIX_P2P_HTTP_BUILD_TESTS)
  include(CTest)
  enable_testing()
  add_subdirectory(tests)
endif()

message(STATUS "------------------------------------------------------")
message(STATUS "vix::p2p_http configured (${PROJECT_VERSION})")
message(STATUS "Mode:              ${_VIX_P2P_HTTP_MODE}")
message(STATUS "Standalone deps:   ${VIX_P2P_HTTP_FETCH_DEPS}")
message(STATUS "Middleware policy: ${VIX_P2P_HTTP_WITH_MIDDLEWARE}")
message(STATUS "------------------------------------------------------")
