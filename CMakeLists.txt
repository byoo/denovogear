##############################################################
#  Denovogear Project Configuration
#
## Project DeNovoGear
PROJECT(DeNovoGear CXX C)
CMAKE_MINIMUM_REQUIRED(VERSION 3.1.0 FATAL_ERROR)

# guard against in-source builds
if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
  message(FATAL_ERROR "In-source builds not allowed. Please make a new directory (called a build directory) and run CMake from there. You may need to remove CMakeCache.txt. ")
endif()

# guard against bad build-type strings
if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release")
endif()

string(TOLOWER "${CMAKE_BUILD_TYPE}" cmake_build_type_tolower)
if(    NOT cmake_build_type_tolower STREQUAL "debug"
   AND NOT cmake_build_type_tolower STREQUAL "release"
   AND NOT cmake_build_type_tolower STREQUAL "relwithdebinfo")
  message(FATAL_ERROR "Unknown build type \"${CMAKE_BUILD_TYPE}\". Allowed values are Debug, Release, RelWithDebInfo (case-insensitive).")
endif()

## Compiling Options

SET(USE_STATIC_LIBS OFF CACHE BOOL "Link against static libraries.")
SET(CMAKE_VERBOSE_MAKEFILE OFF CACHE BOOL "Turn on Verbose Makefiles" FORCE)

## Important Variables

SET(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/Modules")
SET(CMAKE_BUILD_TYPE_INIT "RelWithDebInfo")
SET_PROPERTY(GLOBAL PROPERTY FIND_LIBRARY_USE_LIB64_PATHS ON)

IF(USE_STATIC_LIBS)
ENDIF(USE_STATIC_LIBS)

## Specify Project Information

SET(DNG_NAME "DeNovoGear")
SET(DNG_NAME_FOR_PKG_FILE "denovogear")
SET(DNG_URL "https://github.com/denovogear/denovogear")


################################################################################
# Version Information

## Detect Project Version Information
FIND_PACKAGE(Git)
IF(GIT_FOUND)
  EXECUTE_PROCESS(COMMAND ${GIT_EXECUTABLE} describe --tags --dirty
    RESULT_VARIABLE res_var OUTPUT_VARIABLE GIT_COM_ID
     ERROR_QUIET)
  IF(${res_var} EQUAL 0 )
   	STRING( REPLACE "\n" "" GIT_COMMIT_ID "${GIT_COM_ID}" )
  ENDIF()
ENDIF()

# try directory name
IF(NOT GIT_COMMIT_ID AND PROJECT_SOURCE_DIR MATCHES "${DNG_NAME_FOR_PKG_FILE}-([-a-zA-Z0-9.]*)$" )
  SET(GIT_COMMIT_ID "${CMAKE_MATCH_1}")
ENDIF()

# extract project info
IF(GIT_COMMIT_ID MATCHES
  "^v?([0-9]+)\\.([0-9]+)\\.?([0-9]+)?((-[a-zA-Z]+)?(-[0-9]+)?(-g[0-9A-Fa-f]+)?(-dirty)?)$" )
  IF(NOT DEFINED DNG_VERSION_MAJOR)
    SET(DNG_VERSION_MAJOR "${CMAKE_MATCH_1}")
  ENDIF()
  IF(NOT DEFINED DNG_VERSION_MINOR)
    SET(DNG_VERSION_MINOR "${CMAKE_MATCH_2}")
  ENDIF()
  IF(NOT DEFINED DNG_VERSION_PATCH)
    SET(DNG_VERSION_PATCH "${CMAKE_MATCH_3}")
  ENDIF()
  IF(NOT DEFINED DNG_VERSION_COMMIT)
    STRING(REGEX REPLACE "^-" "" DNG_VERSION_COMMIT "${CMAKE_MATCH_4}")
  ENDIF()
ELSE()
  # We were unable to determine project information, use the default values
  INCLUDE("${CMAKE_CURRENT_SOURCE_DIR}/src/version.h.in")
ENDIF()

SET(DNG_VERSION "${DNG_VERSION_MAJOR}.${DNG_VERSION_MINOR}")
SET(DNG_VERSION_SHORT "${DNG_VERSION_MAJOR}.${DNG_VERSION_MINOR}")
IF(DEFINED DNG_VERSION_PATCH)
  SET(DNG_VERSION "${DNG_VERSION}.${DNG_VERSION_PATCH}")
  SET(DNG_VERSION_SHORT "${DNG_VERSION_SHORT}.${DNG_VERSION_PATCH}")
ENDIF()
IF(DNG_VERSION_COMMIT)
  SET(DNG_VERSION "${DNG_VERSION}-${DNG_VERSION_COMMIT}")
  SET(DNG_VERSION_SHORT "${DNG_VERSION_SHORT}-${DNG_VERSION_COMMIT}")	
ENDIF()

MESSAGE(STATUS "${DNG_NAME} version is ${DNG_VERSION}")

################################################################################
# Packaging Information

SET(CMAKE_DATA_DIR "share/${DNG_NAME_FOR_PKG_FILE}" CACHE STRING
  "Install location for data (relative to prefix).")
SET(CMAKE_DOC_DIR "doc/${DNG_NAME_FOR_PKG_FILE}" CACHE STRING
  "Install location for documentation (relative to prefix).")
SET(CMAKE_MAN_DIR "man" CACHE STRING
  "Install location for man pages (relative to prefix).")
MARK_AS_ADVANCED(CMAKE_DATA_DIR CMAKE_DOC_DIR CMAKE_MAN_DIR)

SET(CPACK_SOURCE_IGNORE_FILES
  "/CVS/" "/\\\\.svn/"  "/\\\\.bzr/" "/\\\\.hg/" "/\\\\.git/" "\\\\.swp$"
  "\\\\.#"  "/#"  ".*~$"
  "/build/[^.]"
  "/CMakeFiles/"  "CMakeCache\\\\.txt$"
  "CPack.*Config\\\\.cmake$"  "cmake_install\\\\.cmake$"
  "install_manifest\\\\.txt$"
  "_CPACK_PACKAGES"  "_CPack_Packages"
  "\\\\.vcproj$"  "\\\\.dir$"  "\\\\.ncb$"  "\\\\.sln$"  "\\\\.suo$"
  "Makefile$" "\\\\.ilk$"  "\\\\.pdb$"
  "\\\\.a$"
)

SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Discover de-novo mutations from deep-sequencing of related individuals.")
SET(CPACK_PACKAGE_VENDOR "Conrad and Cartwright Labs")
SET(CPACK_PACKAGE_EXECUTABLES "denovogear" "DeNovoGear")

SET(CPACK_PACKAGE_INSTALL_DIRECTORY ${DNG_NAME_PKG})
SET(CPACK_PACKAGE_NAME ${DNG_NAME})
SET(CPACK_PACKAGE_NAME_FILE ${DNG_NAME_FOR_PKG_FILE})
SET(CPACK_PACKAGE_VERSION ${DNG_VERSION})
SET(CPACK_PACKAGE_VERSION_MAJOR ${DNG_VERSION_MAJOR})
SET(CPACK_PACKAGE_VERSION_MINOR ${DNG_VERSION_MINOR})
SET(CPACK_PACKAGE_VERSION_PATCH ${DNG_VERSION_PATCH})
SET(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
SET(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/COPYING.txt")

SET(CPACK_SOURCE_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME_FILE}-v${CPACK_PACKAGE_VERSION}")

IF(NOT DEFINED CPACK_SYSTEM_NAME)
  SET(CPACK_SYSTEM_NAME ${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR})
ENDIF(NOT DEFINED CPACK_SYSTEM_NAME)

IF(NOT DEFINED CPACK_PACKAGE_FILE_NAME)
  SET(CPACK_PACKAGE_FILE_NAME "${CPACK_SOURCE_PACKAGE_FILE_NAME}-${CPACK_SYSTEM_NAME}")
ENDIF(NOT DEFINED CPACK_PACKAGE_FILE_NAME)

SET(CPACK_GENERATOR "TGZ;TBZ2")
SET(CPACK_SOURCE_GENERATOR "TGZ;TBZ2") 
SET(CPACK_STRIP_FILES "libexec/dng/*")

INCLUDE(CPack)

INSTALL(FILES README.md COPYING.txt DESTINATION ${CMAKE_DOC_DIR})

ADD_SUBDIRECTORY(src)

########### Add uninstall target ###############
CONFIGURE_FILE(
  "${CMAKE_CURRENT_SOURCE_DIR}/Modules/cmake_uninstall.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
  IMMEDIATE @ONLY)

ADD_CUSTOM_TARGET(uninstall
  "${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake") 
