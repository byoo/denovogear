INCLUDE(CheckIncludeFile)
INCLUDE(CheckFunctionExists)
INCLUDE(CheckLibraryExists)
INCLUDE(GNUInstallDirs)

SET( CMAKE_INSTALL_LIBEXECDIR  "${CMAKE_INSTALL_LIBEXECDIR}/${DNG_NAME_FOR_PKG_FILE}" )

FIND_PACKAGE(ZLIB)
IF(ZLIB_FOUND)
  INCLUDE_DIRECTORIES(${ZLIB_INCLUDE_DIR})
ENDIF(ZLIB_FOUND)

FIND_PACKAGE(Eigen3)
IF(EIGEN3_FOUND)
  INCLUDE_DIRECTORIES(${EIGEN3_INCLUDE_DIR})
  INCLUDE_DIRECTORIES(${EIGEN3_INCLUDE_DIR}/unsupported)
ENDIF(EIGEN3_FOUND)

FIND_PACKAGE(HTSLIB)
IF(HTSLIB_FOUND)
  INCLUDE_DIRECTORIES(${HTSLIB_INCLUDE_DIRS})
  LINK_DIRECTORIES(${HTSLIB_LIBRARY_DIRS})
  IF(HTSLIB_VERSION MATCHES "^([0-9]+)\\.([0-9]+)\\.?([0-9]+)?")
    MATH(EXPR HTSLIB_VERSION_NUM "${CMAKE_MATCH_1}*10000 + ${CMAKE_MATCH_2}*100 + ${CMAKE_MATCH_3}")
    ADD_DEFINITIONS(-DHTSLIB_VERSION_NUM=${HTSLIB_VERSION_NUM})
  ENDIF()
ENDIF(HTSLIB_FOUND)

FIND_PACKAGE(Boost 1.47.0 REQUIRED COMPONENTS program_options filesystem system)
IF(Boost_FOUND)
  ADD_DEFINITIONS(-DBOOST_ALL_NO_LIB -DBOOST_PROGRAM_OPTIONS_NO_LIB)
  INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIRS})
  LINK_DIRECTORIES(${Boost_LIBRARY_DIRS})
  IF(NOT Boost_USE_STATIC_LIBS)
    ADD_DEFINITIONS(-DBOOST_DYN_LINK -DBOOST_PROGRAM_OPTIONS_DYN_LINK)
  ENDIF(NOT Boost_USE_STATIC_LIBS)
#  IF(Boost_VERSION LESS 104700)
#    MESSAGE(STATUS "WARNING: Headers from Boost 1.47+ are needed for successful compiling.") 
#  ENDIF(Boost_VERSION LESS 104700)
ENDIF(Boost_FOUND)


ADD_SUBDIRECTORY(contrib/newmat)
ADD_SUBDIRECTORY(utils)

INCLUDE_DIRECTORIES(BEFORE
	"${CMAKE_CURRENT_SOURCE_DIR}/utils"
	"${CMAKE_CURRENT_SOURCE_DIR}/contrib/newmat"
)

INCLUDE_DIRECTORIES(BEFORE "${CMAKE_CURRENT_BINARY_DIR}")
INCLUDE_DIRECTORIES(BEFORE "${CMAKE_CURRENT_SOURCE_DIR}/include")

ADD_EXECUTABLE(dng-dnm
  dng-dnm.cc
  snpLike.cc indelLike.cc pairLike.cc
  makeLookup.cc
  pedParser.cc
  bcf2Qcall.cc bcf2Paired.cc
)

ADD_EXECUTABLE(dng-phaser
  dng-phaser.cc
  snpLike.cc indelLike.cc pairLike.cc
  makeLookup.cc
  pedParser.cc
  bcf2Qcall.cc bcf2Paired.cc
)

ADD_EXECUTABLE(dng-call
  dng-call.cc
  lib/call.cc lib/likelihood.cc lib/newick.cc lib/pedigree.cc
)
SET_PROPERTY(TARGET dng-call PROPERTY CXX_STANDARD 11)
SET_PROPERTY(TARGET dng-call PROPERTY CXX_STANDARD_REQUIRED ON)

CONFIGURE_FILE(
  "${CMAKE_CURRENT_SOURCE_DIR}/dng.sh.in"
  "${CMAKE_CURRENT_BINARY_DIR}/dng.sh"
  @ONLY)

CONFIGURE_FILE(
  "${CMAKE_CURRENT_SOURCE_DIR}/version.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/version.h"
  @ONLY)

IF(USE_STATIC_LIBS)
  SET_TARGET_PROPERTIES(dng-dnm    PROPERTIES LINK_SEARCH_END_STATIC On)
  SET_TARGET_PROPERTIES(dng-phaser PROPERTIES LINK_SEARCH_END_STATIC On)
  SET_TARGET_PROPERTIES(dng-call   PROPERTIES LINK_SEARCH_END_STATIC On)
ENDIF(USE_STATIC_LIBS)

TARGET_LINK_LIBRARIES(dng-dnm newmat dngutils)
TARGET_LINK_LIBRARIES(dng-dnm ${ZLIB_LIBRARIES})
TARGET_LINK_LIBRARIES(dng-dnm ${CMAKE_REQUIRED_LIBRARIES})

TARGET_LINK_LIBRARIES(dng-phaser newmat dngutils)
TARGET_LINK_LIBRARIES(dng-phaser ${ZLIB_LIBRARIES})
TARGET_LINK_LIBRARIES(dng-phaser ${CMAKE_REQUIRED_LIBRARIES})

TARGET_LINK_LIBRARIES(dng-call ${HTSLIB_LIBRARIES})
TARGET_LINK_LIBRARIES(dng-call ${Boost_LIBRARIES})

INSTALL(TARGETS dng-dnm RUNTIME DESTINATION ${CMAKE_INSTALL_LIBEXECDIR} ${PROJ_INSTALL_EXTRA_ARGS})
INSTALL(TARGETS dng-phaser RUNTIME DESTINATION ${CMAKE_INSTALL_LIBEXECDIR} ${PROJ_INSTALL_EXTRA_ARGS})
INSTALL(TARGETS dng-call RUNTIME DESTINATION ${CMAKE_INSTALL_LIBEXECDIR} ${PROJ_INSTALL_EXTRA_ARGS})
INSTALL(PROGRAMS "${CMAKE_CURRENT_BINARY_DIR}/dng.sh"
  DESTINATION ${CMAKE_INSTALL_BINDIR} RENAME dng)
