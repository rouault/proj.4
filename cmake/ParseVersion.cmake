#
# Copyright © 2020 LambdAurora <aurora42lambda@gmail.com>
#
# This file is part of LambdaCMakeModules.
#
# Licensed under the MIT license. For more information,
# see the LICENSE file.
#

function (parse_version DIR FILENAME VARNAME)
    parse_version_custom_var(${DIR} ${FILENAME} TMP ${VARNAME})
    set(${VARNAME} ${TMP} PARENT_SCOPE)
endfunction ()

function (parse_version_custom_var DIR FILENAME VARNAME VERSION_VAR_NAME)
    parse_version_custom_pattern(${DIR} ${FILENAME} TMP "^#define ${VERSION_VAR_NAME}.*$")
    set(${VARNAME} ${TMP} PARENT_SCOPE)
endfunction ()

function (parse_version_custom_pattern DIR FILENAME VARNAME PATTERN)
    file(STRINGS "${DIR}/${FILENAME}" TMP REGEX ${PATTERN})
    string(REGEX MATCHALL "[0-9]+" TMP ${TMP})
    set(${VARNAME} ${TMP} PARENT_SCOPE)
endfunction ()

function (parse_version_type DIR FILENAME VARNAME)
    parse_version_type_custom_var(${DIR} ${FILENAME} TMP ${VARNAME})
    set(${VARNAME} ${TMP} PARENT_SCOPE)
endfunction ()

function (parse_version_type_custom_var DIR FILENAME VARNAME VERSION_VAR_NAME)
    set(PATTERN "^#define ${VERSION_VAR_NAME}.*$")
    file(STRINGS "${DIR}/${FILENAME}" TMP REGEX ${PATTERN})
    string(REGEX MATCHALL "\"([A-z])+" TMP ${TMP})
    string(REPLACE "\"" "" TMP "${TMP}")
    set(${VARNAME} ${TMP} PARENT_SCOPE)
endfunction ()
