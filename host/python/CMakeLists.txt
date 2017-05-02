#
# Copyright 2017-2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0
#

########################################################################
# This file included, use CMake directory variables
########################################################################

PYTHON_CHECK_MODULE(
    "virtualenv"
    "sys" "hasattr(sys, 'real_prefix')"
    HAVE_PYTHON_VIRTUALENV
)

# Get include dirs
INCLUDE_DIRECTORIES(${PYTHON_INCLUDE_DIRS})
EXECUTE_PROCESS(
    COMMAND "${PYTHON_EXECUTABLE}" -c
    "from __future__ import print_function\ntry: import numpy; print(numpy.get_include(), end='')\nexcept:pass\n"
    OUTPUT_VARIABLE __numpy_path)

FIND_PATH(PYTHON_NUMPY_INCLUDE_DIR numpy/arrayobject.h
          HINTS "${__numpy_path}" "${PYTHON_INCLUDE_PATH}" NO_DEFAULT_PATH)

# Build libpyuhd.so
ADD_LIBRARY(pyuhd SHARED pyuhd.cpp)
TARGET_INCLUDE_DIRECTORIES(pyuhd PUBLIC
    ${PYTHON_NUMPY_INCLUDE_DIR}
    ${CMAKE_SOURCE_DIR}/lib
)
TARGET_LINK_LIBRARIES(pyuhd ${BOOST_PYTHON_LIBRARY} ${Boost_LIBRARIES} uhd)

ADD_CUSTOM_COMMAND(TARGET pyuhd
    POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/libpyuhd.so ${CMAKE_CURRENT_BINARY_DIR}/uhd/libpyuhd.so)

SET(PYUHD_FILES
  ${CMAKE_CURRENT_SOURCE_DIR}/__init__.py
  ${CMAKE_CURRENT_SOURCE_DIR}/types.py
  ${CMAKE_CURRENT_SOURCE_DIR}/usrp.py
  ${CMAKE_CURRENT_SOURCE_DIR}/filters.py
)

SET(SETUP_PY_IN "${CMAKE_CURRENT_SOURCE_DIR}/setup.py.in")
SET(SETUP_PY    "${CMAKE_CURRENT_BINARY_DIR}/setup.py")
SET(OUTPUT      "${CMAKE_CURRENT_BINARY_DIR}/build/timestamp")

CONFIGURE_FILE(${SETUP_PY_IN} ${SETUP_PY})

ADD_CUSTOM_COMMAND(OUTPUT ${OUTPUT}
  COMMAND ${CMAKE_COMMAND} -E copy ${PYUHD_FILES} ${CMAKE_CURRENT_BINARY_DIR}/uhd
  COMMAND ${PYTHON} ${SETUP_PY} -q build
  COMMAND ${CMAKE_COMMAND} -E touch ${OUTPUT}
  DEPENDS ${PYUHD_FILES})

ADD_CUSTOM_TARGET(target ALL DEPENDS ${OUTPUT} pyuhd)
IF(HAVE_PYTHON_VIRTUALENV)
    INSTALL(CODE "execute_process(COMMAND ${PYTHON} ${SETUP_PY} -q install --force)")
ELSE()
    EXECUTE_PROCESS(COMMAND ${PYTHON_EXECUTABLE} -c
      "from distutils import sysconfig; print(sysconfig.get_python_lib(plat_specific=True, prefix=''))"
      OUTPUT_VARIABLE UHD_PYTHON_DIR OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    MESSAGE(STATUS "Utilizing the python install directory: ${CMAKE_INSTALL_PREFIX}/${UHD_PYTHON_DIR}")
    INSTALL(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/uhd DESTINATION ${CMAKE_INSTALL_PREFIX}/${UHD_PYTHON_DIR})
ENDIF()

