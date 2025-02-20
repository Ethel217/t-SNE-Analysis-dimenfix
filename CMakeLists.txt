cmake_minimum_required(VERSION 3.17.0)
 
if(${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.27.0")
    cmake_policy(SET CMP0144 NEW)    # Remove once we set min cmake version accordingly
endif()
 
project("SNE-Analyses" C CXX)
 
# -----------------------------------------------------------------------------
# User options
# -----------------------------------------------------------------------------
option(MV_SNE_USE_ARTIFACTORY_LIBS "Use the prebuilt libraries from artifactory" ON)
option(MV_SNE_USE_AVX "Enable AVX support" OFF)
option(MV_UNITY_BUILD "Combine target source files into batches for faster compilation" OFF)
set(MV_SNE_OPTIMIZATION_LEVEL "2" CACHE STRING "Optimization level for all targets in release builds, e.g. 0, 1, 2")
#set(VCPKG_BASE_PATH "{PATH_TO_VCPKG_DIR}/vcpkg" CACHE PATH "Base path for vcpkg")
#set(HDILIB_BASE_PATH "{PATH_TO_LOCAL_HDILIB_DIR}/HDILib-dimenfix" CACHE PATH "Base path for hdilib")
if(NOT MV_SNE_USE_ARTIFACTORY_LIBS)
    # check if VCPKG_BASE_PATH and HDILIB_BASE_PATH are defined 
    if(NOT VCPKG_BASE_PATH)
        message(FATAL_ERROR "VCPKG_BASE_PATH is required but not defined!")
    endif()

    if(NOT HDILIB_BASE_PATH)
        message(FATAL_ERROR "HDILIB_BASE_PATH is required but not defined!")
    endif()
endif()

# -----------------------------------------------------------------------------
# CMake options
# -----------------------------------------------------------------------------
set(VERBOSE TRUE)
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
 
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
 
# macros for linking both prebuilt artifactory libraries and locally built ones
include(CMakeLinkUtilities)
 
# set optimization in release mode
include(CMakeSetOptimizationLevel)
 
if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /DWIN32 /EHsc /MP  /permissive- /Zc:__cplusplus")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MDd")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MD")
endif()
 
# -----------------------------------------------------------------------------
# Install dependency libraries
# -----------------------------------------------------------------------------
 
# HDILib, flann and lz4 are available prebuilt in the lkeb-artifactory
if (MV_SNE_USE_ARTIFACTORY_LIBS AND NOT MV_SNE_ARTIFACTORY_LIBS_INSTALLED) 
    set(LIBRARY_INSTALL_DIR ${PROJECT_BINARY_DIR})
    message(STATUS "Installing artifactory packages to: ${LIBRARY_INSTALL_DIR}")
 
    set(HDILib_VERSION 1.2.9)
    set(flann_VERSION 1.9.2)
    set(lz4_VERSION 1.10.0)
 
    include(FetchContent)
    FetchContent_Declare(cmakesupport
        GIT_REPOSITORY https://github.com/ManiVaultStudio/github-actions
        GIT_TAG main
        GIT_SHALLOW TRUE
        SOURCE_SUBDIR CMakeSupport
    )
    FetchContent_MakeAvailable(cmakesupport)
 
    message(STATUS "FetchContent: ${cmakesupport_SOURCE_DIR} ${cmakesupport_BINARY_DIR}")
    include("${cmakesupport_SOURCE_DIR}/CMakeSupport/InstallArtifactoryPackage.cmake")
 
    install_artifactory_package(HDILib ${HDILib_VERSION} biovault TRUE) 
    install_artifactory_package(flann ${flann_VERSION} lkeb TRUE)
    install_artifactory_package(lz4 ${lz4_VERSION} lkeb TRUE)
	set(lz4_ROOT "${LIBRARY_INSTALL_DIR}/lz4/lib/cmake/lz4" CACHE PATH "Location of lz4 cmake file")
	set(flann_ROOT "${LIBRARY_INSTALL_DIR}/flann/lib/cmake/flann" CACHE PATH "Location of flann cmake file")
	set(HDILIB_ROOT "${LIBRARY_INSTALL_DIR}/HDILib/lib/cmake/HDILib" CACHE PATH "Location of HDILib cmake file")
	set(MV_SNE_ARTIFACTORY_LIBS_INSTALLED TRUE CACHE BOOL "Use the prebuilt libraries from artifactory" FORCE)
else()
	set(hdf5_DIR "${VCPKG_BASE_PATH}/installed/x64-windows-static-md/share/hdf5" CACHE PATH "Custom path for hdf5")
	set(lz4_DIR "${VCPKG_BASE_PATH}/installed/x64-windows-static-md/share/lz4" CACHE PATH "Custom path for lz4")
	set(flann_DIR "${VCPKG_BASE_PATH}/installed/x64-windows-static-md/share/flann" CACHE PATH "Custom path for flann")
	set(HDILIB_ROOT "${HDILIB_BASE_PATH}/install/lib/cmake/HDILib" CACHE PATH "Custom path for HDILIB_ROOT")
	set(HDILib_DIR "${HDILIB_BASE_PATH}/install/lib/cmake/HDILib" CACHE PATH "Custom path for HDILib_DIR")
	set(OpenCV_DIR "${VCPKG_BASE_PATH}/installed/x64-windows-static-md/share/opencv4" CACHE PATH "Custom path for OpenCV")
	set(Protobuf_DIR "${VCPKG_BASE_PATH}/installed/x64-windows-static-md/share/protobuf" CACHE PATH "Custom path for Protobuf")
	set(Protobuf_PROTOC_EXECUTABLE "${VCPKG_BASE_PATH}/installed/x64-windows-static-md/tools/protobuf/protoc.exe" CACHE FILEPATH "Path to Protobuf's protoc")
	set(absl_DIR "${VCPKG_BASE_PATH}/installed/x64-windows-static-md/share/absl" CACHE PATH "Custom path for absl")
	set(utf8_range_DIR "${VCPKG_BASE_PATH}/installed/x64-windows-static-md/share/utf8_range" CACHE PATH "Custom path for utf8_range")
	set(TIFF_INCLUDE_DIR "${VCPKG_BASE_PATH}/installed/x64-windows-static-md/include" CACHE PATH "Custom path for TIFF include directory")
	set(TIFF_LIBRARY "${VCPKG_BASE_PATH}/installed/x64-windows-static-md/lib/tiff.lib" CACHE FILEPATH "Custom path for TIFF library")
	set(quirc_DIR "${VCPKG_BASE_PATH}/installed/x64-windows-static-md/share/quirc" CACHE PATH "Custom path for quirc")
endif()
# -----------------------------------------------------------------------------
# Dependencies
# -----------------------------------------------------------------------------
 
find_package(Qt6 COMPONENTS Widgets WebEngineWidgets REQUIRED)
 
find_package(ManiVault COMPONENTS Core PointData ImageData CONFIG)
 
find_package(lz4 CONFIG)
if(NOT ${lz4_FOUND})
    find_package(PkgConfig REQUIRED)
 
    pkg_check_modules(lz4 REQUIRED IMPORTED_TARGET liblz4)
    set(LZ4_TARGET PkgConfig::lz4)
endif()
 
if(NOT ${lz4_FOUND})
    message (FATAL_ERROR "lz4 NOT found")
endif()
message (STATUS "Found lz4 version ${lz4_VERSION}")
 
find_package(flann CONFIG)
if(NOT ${flann_FOUND})
    find_package(PkgConfig REQUIRED)
 
    pkg_check_modules(flann REQUIRED flann)
    set(FLANN_TARGET flann_cpp)
    set(flann_INCLUDE_DIR "${flann_INCLUDE_DIRS}/flann")
endif()
 
if(NOT ${flann_FOUND})
    message (FATAL_ERROR "flann NOT found")
endif()
message (STATUS "Found flann version ${flann_VERSION}")
 
if(NOT HDILIB_ROOT)
    message(FATAL_ERROR "Define HDILIB_ROOT")
endif()
 
find_package(HDILib COMPONENTS hdidimensionalityreduction hdiutils hdidata CONFIG REQUIRED PATHS ${HDILIB_ROOT})
 
if(${HDILib_FOUND})
    message(STATUS "HDILib found at ${HDILIB_ROOT} with ${HDILib_LINK_LIBS}")
endif()
 
find_package(OpenGL REQUIRED)
find_package(OpenMP)
 
if(OpenMP_CXX_FOUND)
    message(STATUS "Found OpenMP")
endif()
 
if(UNIX)
    message(STATUS "pThreads for Linux")
    find_package(Threads REQUIRED)
endif(UNIX)
 
set(THIRD_PARTY_JSON
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/json/nlohmann/json.hpp
)
 
# Checks whether this is the first CMake run for this project.
# If so, sets the CMake Install Prefix to the ManiVault install directory.
# This if-statement must be placed after the project() command to work.
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX ${MV_INSTALL_DIR} CACHE PATH "..." FORCE)
endif()
 
# -----------------------------------------------------------------------------
# Project targets
# -----------------------------------------------------------------------------
 
# Source files shared by both Hsne and Tsne plugins
add_subdirectory(src/Common)
 
include(CMakeTsneProject)
include(CMakeHsneProject)