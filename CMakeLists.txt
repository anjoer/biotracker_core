cmake_minimum_required(VERSION 2.8.7)

if (POLICY CMP0020)
	cmake_policy (SET CMP0020 NEW)
endif(POLICY CMP0020)

# Do not try to resolve simple strings as if they were variables.
if (POLICY CMP0054)
    cmake_policy (SET CMP0054 NEW)
endif()

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

# add custom cmake packages
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

project(biotracker_core)

#-----------------------------------------------------------------------
# CPM configuration
#-----------------------------------------------------------------------
set(CPM_MODULE_NAME BioTrackerLib)
set(CPM_LIB_TARGET_NAME ${CPM_MODULE_NAME})

if ((DEFINED CPM_DIR) AND (DEFINED CPM_UNIQUE_ID) AND (DEFINED CPM_TARGET_NAME))
  set(CPM_LIB_TARGET_NAME ${CPM_TARGET_NAME})
  set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CPM_DIR})
  include(CPM)
else()
  set(CPM_DIR "${CMAKE_CURRENT_BINARY_DIR}/cpm-packages" CACHE TYPE STRING)
  find_package(Git)
  if(NOT GIT_FOUND)
    message(FATAL_ERROR "CPM requires Git.")
  endif()
  if (NOT EXISTS ${CPM_DIR}/CPM.cmake)
    message(STATUS "Cloning repo (https://github.com/iauns/cpm)")
    execute_process(
      COMMAND "${GIT_EXECUTABLE}" clone https://github.com/iauns/cpm ${CPM_DIR}
      RESULT_VARIABLE error_code
      OUTPUT_QUIET ERROR_QUIET)
    if(error_code)
      message(FATAL_ERROR "CPM failed to get the hash for HEAD")
    endif()
  endif()
  include(${CPM_DIR}/CPM.cmake)
endif()

CPM_AddModule("cpm_cereal"
    GIT_REPOSITORY https://github.com/BioroboticsLab/cpm_cereal.git
    GIT_TAG 874092ae503eec016192ddd6ff17ae23839085be
    EXPORT_MODULE true
)

if(NOT DEFINED CMAKECONFIG_PATH)
    CPM_AddModule("cmakeconfig"
        GIT_REPOSITORY "https://github.com/BioroboticsLab/cmakeconfig.git"
        GIT_TAG "master")
else()
    CPM_AddModule("cmakeconfig"
        SOURCE_DIR "${CMAKECONFIG_PATH}")
endif()

CPM_InitModule(${CPM_MODULE_NAME})

biorobotics_config()
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/ ${CMAKE_MODULE_PATH})

# add git hooks
file(GLOB HOOK_SCRIPTS githooks/*)
if (HOOK_SCRIPTS)
    file(COPY ${HOOK_SCRIPTS} DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/.git/hooks)
endif (HOOK_SCRIPTS)

# project dependecies
#set(OpenCV_STATIC ON)
find_package(OpenCV REQUIRED)
find_package(OpenGL REQUIRED)
find_package(Qt5Widgets 5.4 REQUIRED)
find_package(Qt5OpenGL 5.4 REQUIRED)
find_package(Threads REQUIRED)
find_package(ZMQ REQUIRED)

set(Boost_USE_STATIC_LIBS OFF)
find_package(Boost COMPONENTS filesystem serialization system REQUIRED)

# Qt options
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
add_definitions(${Qt5Widgets_DEFINITIONS})
add_definitions(-DQT_NO_KEYWORDS)
CPM_ExportAdditionalDefinition(-DQT_NO_KEYWORDS)

# Disable warnings
add_definitions(-D_SCL_SECURE_NO_WARNINGS)
# Specify that we want to export symbols to a DLL (as opposed to importing them from a DLL).
add_definitions(-DBUILD_BIOTRACKER_DLL)

# include external dependecies as SYSTEM headers to prevent warnings from
# external files. unfortunately, this has no effect when using MSVC.
include_directories(
    ${PROJECT_SOURCE_DIR}
    SYSTEM ${OpenCV_INCLUDE_DIRS}
    SYSTEM ${Qt5OpenGL_INCLUDE_DIRS}
    SYSTEM ${Qt5Widgets_INCLUDE_DIRS}
    SYSTEM ${Boost_INCLUDE_DIRS}
    SYSTEM ${ZMQ_INCLUDE_DIRS}
    SYSTEM ${CMAKE_CURRENT_BINARY_DIR}/
)

link_directories(
    ${Boost_LIBRARY_DIRS}
)

add_subdirectory(biotracker)
add_subdirectory(doc)

option(BIOTRACKER_BUILD_TESTS "BIOTRACKER_BUILD_TESTS" On)
if (BIOTRACKER_BUILD_TESTS)
    enable_testing()
    add_subdirectory(test EXCLUDE_FROM_ALL)
endif()
