#
# Copyright © 2019 LambdAurora <aurora42lambda@gmail.com>
#
# This file is part of LambdaCMakeModules.
#
# Licensed under the MIT license. For more information,
# see the LICENSE file.
#

#.rst:
# Findnlohmann_json
# ----------------
#
# Find the nlohmann_json library.
#
# IMPORTED Targets
# ^^^^^^^^^^^^^^^^
#
# This module defines the :prop_tgt:`IMPORTED` target ``nlohmann::json``,
# if nlohmann_json has been found.
#
# Result Variables
# ^^^^^^^^^^^^^^^^
#
# This module defines the following variables:
#
# ::
#
#   NLOHMANN_JSON_INCLUDE_DIRS - Include directories for nlohmann_json.
#   NLOHMANN_JSON_VERSION - Version of nlohmann_json.
#   nlohmann_json_FOUND - True if nlohmann_json has been found and can be used.

include(FindPackageHandleStandardArgs)

# Look for the header file.
find_path(NLOHMANN_JSON_INCLUDE_DIR
    NAMES
        nlohmann/json.hpp
    HINTS
        "${NLOHMANN_JSON_LOCATION}/include"
        "$ENV{NLOHMANN_JSON_LOCATION}/include"
    PATHS
        "$ENV{PROGRAMFILES}/nlohmann/include"
        "$ENV{PROGRAMFILES}/nlohmann/json/include"
        "$ENV{PROGRAMFILES}/nlohmann_json/include"
        /usr/include/
        /usr/local/include/
        /opt/local/include/
        /sw/include/
    DOC
        "The directory where headers files of nlohmann_json resides")
mark_as_advanced(NLOHMANN_JSON_INCLUDE_DIR)

if (NLOHMANN_JSON_INCLUDE_DIR)
    # Tease the NLOHMANN_JSON_VERSION numbers from the lib headers
    include(ParseVersion)

    parse_version(${NLOHMANN_JSON_INCLUDE_DIR} nlohmann/json.hpp NLOHMANN_JSON_VERSION_MAJOR)
    parse_version(${NLOHMANN_JSON_INCLUDE_DIR} nlohmann/json.hpp NLOHMANN_JSON_VERSION_MINOR)
    parse_version(${NLOHMANN_JSON_INCLUDE_DIR} nlohmann/json.hpp NLOHMANN_JSON_VERSION_PATCH)

    set(NLOHMANN_JSON_VERSION "${NLOHMANN_JSON_VERSION_MAJOR}.${NLOHMANN_JSON_VERSION_MINOR}.${NLOHMANN_JSON_VERSION_PATCH}")
endif (NLOHMANN_JSON_INCLUDE_DIR)

find_package_handle_standard_args(nlohmann_json
    REQUIRED_VARS NLOHMANN_JSON_INCLUDE_DIR
    VERSION_VAR NLOHMANN_JSON_VERSION)

if (nlohmann_json_FOUND)
  set(NLOHMANN_JSON_INCLUDE_DIRS ${NLOHMANN_JSON_INCLUDE_DIR})

  if (NOT TARGET nlohmann::json)
    add_library(nlohmann::json UNKNOWN IMPORTED)
    set_target_properties(nlohmann::json PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES "${NLOHMANN_JSON_INCLUDE_DIRS}")
  endif ()
endif ()
