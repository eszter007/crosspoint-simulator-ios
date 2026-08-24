cmake_minimum_required(VERSION 3.16)
project(crosspoint-sim-ios CXX C)

# CMake build for the CrossPoint simulator.
#
# The simulator normally builds through PlatformIO, as a library the firmware
# pulls in via lib_deps (see the repository root's sample-platformio-*.ini).
# PlatformIO has no iOS platform, so an iPhone build needs its own build system;
# this one compiles exactly what the PlatformIO env does -- same sources, same
# flags -- and additionally knows how to produce an iOS app bundle.
#
# Desktop (a native binary, same as the PlatformIO env):
#   cmake -S ios -B build -DCROSSPOINT_FIRMWARE_ROOT=../crosspoint-reader
#   cmake --build build
#
# iPhone (see ios/README.md):
#   cmake -S ios -B build-ios -G Xcode -DCMAKE_SYSTEM_NAME=iOS \
#         -DCROSSPOINT_FIRMWARE_ROOT=../crosspoint-reader \
#         -DSDL2_SOURCE_DIR=$HOME/src/SDL2

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD 11)

set(SIMULATOR_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/..")
get_filename_component(SIMULATOR_ROOT "${SIMULATOR_ROOT}" ABSOLUTE)

# --- firmware being simulated --------------------------------------------------
if(NOT DEFINED CROSSPOINT_FIRMWARE_ROOT)
  set(CROSSPOINT_FIRMWARE_ROOT "${SIMULATOR_ROOT}/../crosspoint-reader")
endif()
# EXISTS is only well-defined for absolute paths; a relative value would resolve
# against the process's working directory rather than this file's.
get_filename_component(CROSSPOINT_FIRMWARE_ROOT "${CROSSPOINT_FIRMWARE_ROOT}" ABSOLUTE
                       BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
if(NOT EXISTS "${CROSSPOINT_FIRMWARE_ROOT}/src/main.cpp")
  message(FATAL_ERROR
    "No firmware at ${CROSSPOINT_FIRMWARE_ROOT}. Pass -DCROSSPOINT_FIRMWARE_ROOT=/path/to/crosspoint-reader.")
endif()

if(NOT DEFINED FREEINK_SDK_ROOT)
  set(FREEINK_SDK_ROOT "${CROSSPOINT_FIRMWARE_ROOT}/freeink-sdk")
endif()
get_filename_component(FREEINK_SDK_ROOT "${FREEINK_SDK_ROOT}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
if(NOT EXISTS "${FREEINK_SDK_ROOT}/libs/ui/FreeInkUI")
  message(FATAL_ERROR
    "No FreeInk SDK at ${FREEINK_SDK_ROOT}. Run: git -C ${CROSSPOINT_FIRMWARE_ROOT} submodule update --init --recursive")
endif()

message(STATUS "Firmware:    ${CROSSPOINT_FIRMWARE_ROOT}")
message(STATUS "FreeInk SDK: ${FREEINK_SDK_ROOT}")

# --- device selection ----------------------------------------------------------
# Mirrors the PlatformIO envs in sample-platformio-*.ini. Defaults to the X4 Pro:
# it is the touch board, and a phone with no keyboard is a poor fit for the
# button-only devices.
set(SIMULATOR_DEVICE "x4pro" CACHE STRING "Device to simulate: x4, x3, x4pro, sticky, papermono")
set_property(CACHE SIMULATOR_DEVICE PROPERTY STRINGS x4 x3 x4pro sticky papermono)

set(SIMULATOR_DISPLAY "default" CACHE STRING "Panel controller override: default, uc8179, uc8279")
set_property(CACHE SIMULATOR_DISPLAY PROPERTY STRINGS default uc8179 uc8279)

set(DEVICE_DEFINES)
if(SIMULATOR_DEVICE STREQUAL "x4")
  list(APPEND DEVICE_DEFINES FREEINK_DEVICE_X4=1)
elseif(SIMULATOR_DEVICE STREQUAL "x3")
  list(APPEND DEVICE_DEFINES SIMULATOR_DEVICE_X3 FREEINK_DEVICE_X3=1)
elseif(SIMULATOR_DEVICE STREQUAL "x4pro")
  list(APPEND DEVICE_DEFINES SIMULATOR_DEVICE_X4_PRO FREEINK_DEVICE_X4PRO=1)
elseif(SIMULATOR_DEVICE STREQUAL "sticky")
  list(APPEND DEVICE_DEFINES SIMULATOR_DEVICE_STICKY FREEINK_DEVICE_STICKY=1)
elseif(SIMULATOR_DEVICE STREQUAL "papermono")
  list(APPEND DEVICE_DEFINES SIMULATOR_DEVICE_PAPERMONO FREEINK_DEVICE_PAPERMONO=1)
else()
  message(FATAL_ERROR "SIMULATOR_DEVICE must be x4, x3, x4pro, sticky or papermono (got '${SIMULATOR_DEVICE}')")
endif()

if(SIMULATOR_DISPLAY STREQUAL "uc8179")
  list(APPEND DEVICE_DEFINES SIMULATOR_DISPLAY_UC8179)
elseif(SIMULATOR_DISPLAY STREQUAL "uc8279")
  list(APPEND DEVICE_DEFINES SIMULATOR_DISPLAY_UC8279)
elseif(NOT SIMULATOR_DISPLAY STREQUAL "default")
  message(FATAL_ERROR "SIMULATOR_DISPLAY must be default, uc8179 or uc8279 (got '${SIMULATOR_DISPLAY}')")
endif()

message(STATUS "Simulating:  ${SIMULATOR_DEVICE} (panel controller: ${SIMULATOR_DISPLAY})")

# --- platform ------------------------------------------------------------------
if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
  set(CROSSPOINT_IOS TRUE)
  enable_language(OBJCXX Swift)
endif()

# SDL2. iOS has no system SDL2 to find, so it is built from an SDL2 source
# checkout as a subproject; SDL2's own CMake build supplies both the static
# library and the iOS entry-point shim.
set(SDL2_FROM_SOURCE FALSE)
if(DEFINED SDL2_SOURCE_DIR)
  if(NOT EXISTS "${SDL2_SOURCE_DIR}/CMakeLists.txt")
    message(FATAL_ERROR "SDL2_SOURCE_DIR is not an SDL2 checkout: ${SDL2_SOURCE_DIR}")
  endif()
  set(SDL_SHARED OFF CACHE BOOL "" FORCE)
  set(SDL_STATIC ON CACHE BOOL "" FORCE)
  set(SDL_TEST OFF CACHE BOOL "" FORCE)
  add_subdirectory("${SDL2_SOURCE_DIR}" "${CMAKE_BINARY_DIR}/sdl2-build" EXCLUDE_FROM_ALL)
  set(SDL2_FROM_SOURCE TRUE)
elseif(CROSSPOINT_IOS)
  message(FATAL_ERROR "iOS builds need -DSDL2_SOURCE_DIR=/path/to/SDL2 (an SDL2, not SDL3, source checkout).")
else()
  find_package(SDL2 QUIET)
  if(NOT SDL2_FOUND)
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
      pkg_check_modules(SDL2 sdl2)
      set(SDL2_USE_PKGCONFIG ON)
    endif()
  endif()
  if(NOT SDL2_FOUND)
    message(FATAL_ERROR "SDL2 not found. Need SDL2 (not SDL3), or pass -DSDL2_SOURCE_DIR=/path/to/SDL2.")
  endif()
endif()

# --- generated firmware sources ------------------------------------------------
# PlatformIO runs these as pre: extra_scripts. git_branch.py is skipped: it only
# injects a version string, which this build sets directly below.
find_package(Python3 COMPONENTS Interpreter REQUIRED)

if(EXISTS "${CROSSPOINT_FIRMWARE_ROOT}/scripts/gen_i18n.py")
  execute_process(
    COMMAND "${Python3_EXECUTABLE}" "${CROSSPOINT_FIRMWARE_ROOT}/scripts/gen_i18n.py"
            "${CROSSPOINT_FIRMWARE_ROOT}/lib/I18n/translations" "${CROSSPOINT_FIRMWARE_ROOT}/lib/I18n/"
    WORKING_DIRECTORY "${CROSSPOINT_FIRMWARE_ROOT}"
    RESULT_VARIABLE gen_i18n_result
    OUTPUT_QUIET)
  if(NOT gen_i18n_result EQUAL 0)
    message(FATAL_ERROR "gen_i18n.py failed (${gen_i18n_result})")
  endif()
endif()

if(EXISTS "${CROSSPOINT_FIRMWARE_ROOT}/scripts/build_html.py")
  execute_process(
    COMMAND "${CMAKE_COMMAND}" -E env "PYTHONPATH=" "${Python3_EXECUTABLE}"
            "${CROSSPOINT_FIRMWARE_ROOT}/scripts/build_html.py"
    WORKING_DIRECTORY "${CROSSPOINT_FIRMWARE_ROOT}"
    OUTPUT_QUIET)
endif()

# --- sources -------------------------------------------------------------------
file(GLOB SIMULATOR_SOURCES "${SIMULATOR_ROOT}/src/*.cpp")

# src/SimulatorPlatform.cpp compiles to nothing under SIMULATOR_IOS; the
# Objective-C implementation takes its place.
if(CROSSPOINT_IOS)
  list(APPEND SIMULATOR_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/sim_ios_platform.m")
endif()

# Firmware application sources, minus what build_src_filter drops in the
# PlatformIO env: firmware-update paths stay non-destructive in the simulator.
file(GLOB_RECURSE FIRMWARE_SOURCES
  "${CROSSPOINT_FIRMWARE_ROOT}/src/*.cpp"
  "${CROSSPOINT_FIRMWARE_ROOT}/src/*.c")
list(FILTER FIRMWARE_SOURCES EXCLUDE REGEX ".*/network/FirmwareFlasher\\.cpp$")
list(FILTER FIRMWARE_SOURCES EXCLUDE REGEX ".*/network/OtaBootSwitch\\.cpp$")
list(FILTER FIRMWARE_SOURCES EXCLUDE REGEX ".*/network/OtaUpdater\\.cpp$")
list(FILTER FIRMWARE_SOURCES EXCLUDE REGEX ".*/platform/skip_efuse_blk_check\\.c$")

# Firmware libraries. lib/hal is the on-device HAL the simulator replaces
# (lib_ignore = hal); PNGdec/JPEGDEC are ignored too, the simulator decoding
# images with stb_image instead.
#
# Follows PlatformIO's library layout rule: a library with a src/ directory
# exposes only that, and anything beside it is support material the src/ files
# pull in themselves. lib/miniz is exactly this -- src/miniz_impl.c #includes
# third_party/miniz.c to compile it under the project's config -- so compiling
# the tree wholesale defines every miniz symbol twice.
set(FIRMWARE_LIB_SOURCES)
file(GLOB FIRMWARE_LIB_DIRS_ALL LIST_DIRECTORIES true "${CROSSPOINT_FIRMWARE_ROOT}/lib/*")
foreach(lib_dir ${FIRMWARE_LIB_DIRS_ALL})
  if(NOT IS_DIRECTORY "${lib_dir}")
    continue()
  endif()
  get_filename_component(lib_name "${lib_dir}" NAME)
  if(lib_name STREQUAL "hal")
    continue()
  endif()
  if(IS_DIRECTORY "${lib_dir}/src")
    file(GLOB_RECURSE lib_sources "${lib_dir}/src/*.cpp" "${lib_dir}/src/*.c")
  else()
    file(GLOB_RECURSE lib_sources "${lib_dir}/*.cpp" "${lib_dir}/*.c")
  endif()
  list(APPEND FIRMWARE_LIB_SOURCES ${lib_sources})
endforeach()

# FreeInk SDK pieces the PlatformIO env symlinks in as libraries.
file(GLOB_RECURSE SDK_SOURCES
  "${FREEINK_SDK_ROOT}/libs/ui/FreeInkUI/src/*.cpp"
  "${FREEINK_SDK_ROOT}/libs/assets/Icons/src/*.cpp")

# --- third-party Arduino libraries ---------------------------------------------
# PlatformIO resolves these from lib_deps; CMake fetches them at the same pinned
# versions. Point CROSSPOINT_THIRDPARTY_DIR at a directory holding ArduinoJson/
# and AnimatedGIF/ to build offline instead.
include(FetchContent)

set(CROSSPOINT_THIRDPARTY_DIR "" CACHE PATH "Directory holding pre-fetched ArduinoJson/ and AnimatedGIF/")

function(crosspoint_provide_library name git_repo git_tag out_dir)
  if(CROSSPOINT_THIRDPARTY_DIR AND EXISTS "${CROSSPOINT_THIRDPARTY_DIR}/${name}")
    set(${out_dir} "${CROSSPOINT_THIRDPARTY_DIR}/${name}" PARENT_SCOPE)
    return()
  endif()
  FetchContent_Declare(${name} GIT_REPOSITORY "${git_repo}" GIT_TAG "${git_tag}" GIT_SHALLOW TRUE)
  FetchContent_MakeAvailable(${name})
  string(TOLOWER "${name}" lower_name)
  set(${out_dir} "${${lower_name}_SOURCE_DIR}" PARENT_SCOPE)
endfunction()

crosspoint_provide_library(ArduinoJson "https://github.com/bblanchon/ArduinoJson.git" "v7.4.2" ARDUINOJSON_DIR)
crosspoint_provide_library(AnimatedGIF "https://github.com/bitbank2/AnimatedGIF.git"
                           "d01888f0255fd0781fc2b8600b9111b14c999584" ANIMATEDGIF_DIR)

file(GLOB ANIMATEDGIF_SOURCES "${ANIMATEDGIF_DIR}/src/*.cpp")

# --- target --------------------------------------------------------------------
set(IOS_RESOURCES)
set(IOS_SOURCES)
if(CROSSPOINT_IOS)
  set(IOS_RESOURCES "${CMAKE_CURRENT_SOURCE_DIR}/LaunchScreen.storyboard")
  set(IOS_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/NativeControls.swift"
    "${CMAKE_CURRENT_SOURCE_DIR}/SwiftControlsBridge.mm")
endif()

add_executable(crosspoint_simulator MACOSX_BUNDLE
  ${SIMULATOR_SOURCES}
  ${FIRMWARE_SOURCES}
  ${FIRMWARE_LIB_SOURCES}
  ${SDK_SOURCES}
  ${ANIMATEDGIF_SOURCES}
  ${IOS_SOURCES}
  ${IOS_RESOURCES}
)

# Every firmware lib directory is on the include path, matching how PlatformIO's
# library dependency finder exposes them.
set(FIRMWARE_INCLUDES
  "${CMAKE_CURRENT_SOURCE_DIR}"
  "${SIMULATOR_ROOT}/src"
  "${CROSSPOINT_FIRMWARE_ROOT}/src"
  "${CROSSPOINT_FIRMWARE_ROOT}/lib"
  "${ARDUINOJSON_DIR}/src"
  "${ANIMATEDGIF_DIR}/src"
)
# Every directory inside a firmware library that holds a header goes on the
# include path, not just the library root. PlatformIO's dependency finder does
# the same at lib_ldf_mode = deep+, and firmware code relies on it: a source in
# lib/Epub/Epub/blocks/ includes "Kinsoku.h" from lib/Epub/Epub/, which a
# roots-only path cannot resolve.
file(GLOB_RECURSE FIRMWARE_HEADERS
  "${CROSSPOINT_FIRMWARE_ROOT}/lib/*.h"
  "${CROSSPOINT_FIRMWARE_ROOT}/lib/*.hpp")
set(FIRMWARE_HEADER_DIRS)
foreach(header ${FIRMWARE_HEADERS})
  get_filename_component(header_dir "${header}" DIRECTORY)
  list(APPEND FIRMWARE_HEADER_DIRS "${header_dir}")
endforeach()
list(REMOVE_DUPLICATES FIRMWARE_HEADER_DIRS)
# lib/hal is replaced by the simulator's own HAL and must not shadow it.
list(FILTER FIRMWARE_HEADER_DIRS EXCLUDE REGEX "${CROSSPOINT_FIRMWARE_ROOT}/lib/hal(/.*)?$")
# lib/Epub/Epub/blocks/ holds a Block.h, which shadows the Objective-C blocks
# runtime header of that name when UIKit pulls it in. Nothing needs the
# directory on the path: every reference to its headers is quoted, so it
# resolves relative to the including file.
# ponytail: one name, excluded by hand. If a second firmware header ever
# collides with an SDK one, compile the Objective-C sources in a separate
# target that carries no firmware includes.
list(FILTER FIRMWARE_HEADER_DIRS EXCLUDE REGEX "${CROSSPOINT_FIRMWARE_ROOT}/lib/Epub/Epub/blocks$")
list(APPEND FIRMWARE_INCLUDES ${FIRMWARE_HEADER_DIRS})
file(GLOB SDK_INCLUDE_DIRS "${FREEINK_SDK_ROOT}/libs/*/*/include")
list(APPEND FIRMWARE_INCLUDES ${SDK_INCLUDE_DIRS})

target_include_directories(crosspoint_simulator PRIVATE ${FIRMWARE_INCLUDES})

target_compile_definitions(crosspoint_simulator PRIVATE
  ${DEVICE_DEFINES}
  SIMULATOR
  CROSSPOINT_SIMULATOR_PROJECT_WEBSERVER
  CROSSPOINT_VERSION="dev-simulator"
  ENABLE_SERIAL_LOG
  LOG_LEVEL=2
  EINK_DISPLAY_SINGLE_BUFFER_MODE=1
  MINIZ_NO_ZLIB_COMPATIBLE_NAMES=1
  XML_GE=0
  XML_CONTEXT_BYTES=1024
  USE_UTF8_LONG_NAMES=1
  PNG_MAX_BUFFERED_PIXELS=16416
  DISABLE_FS_H_WARNING=1
  DESTRUCTOR_CLOSES_FILE=1
  # A host has one address space, so the AVR program-memory accessors are plain
  # reads. sample-platformio-*.ini suggests the same define for native decoder
  # builds; AnimatedGIF needs it unconditionally.
  memcpy_P=memcpy
)

# -Wno-c++11-narrowing, as the PlatformIO env passes.
target_compile_options(crosspoint_simulator PRIVATE
  $<$<COMPILE_LANGUAGE:CXX>:-Wno-narrowing>)

if(SDL2_FROM_SOURCE)
  # SDL2main provides the iOS entry point that stands up UIApplication before
  # calling SDL_main.
  target_link_libraries(crosspoint_simulator PRIVATE SDL2::SDL2main SDL2::SDL2-static)
elseif(SDL2_USE_PKGCONFIG)
  target_include_directories(crosspoint_simulator PRIVATE ${SDL2_INCLUDE_DIRS})
  target_link_directories(crosspoint_simulator PRIVATE ${SDL2_LIBRARY_DIRS})
  target_link_libraries(crosspoint_simulator PRIVATE ${SDL2_LIBRARIES})
else()
  target_link_libraries(crosspoint_simulator PRIVATE SDL2::SDL2)
endif()

# Desktop simulator links curl and, on Linux, OpenSSL for MD5 (see the README's
# prerequisites). iOS does neither: no linkable libcurl in the SDK, and the
# simulator's own MD5 fallback is used instead.
if(NOT CROSSPOINT_IOS)
  find_package(CURL REQUIRED)
  target_link_libraries(crosspoint_simulator PRIVATE CURL::libcurl)
  if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    find_package(OpenSSL REQUIRED)
    target_link_libraries(crosspoint_simulator PRIVATE OpenSSL::Crypto)
  endif()
endif()

# --- iOS app bundle ------------------------------------------------------------
if(CROSSPOINT_IOS)
  target_compile_definitions(crosspoint_simulator PRIVATE SIMULATOR_IOS=1)
  # UniformTypeIdentifiers supplies UTTypeFolder for the folder picker.
  target_link_libraries(crosspoint_simulator PRIVATE
    "-framework UIKit" "-framework SwiftUI" "-framework Foundation"
    "-framework UniformTypeIdentifiers")

  set_source_files_properties(${IOS_RESOURCES} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
  set(MACOSX_BUNDLE_GUI_IDENTIFIER "org.crosspoint.simulator" CACHE STRING "iOS bundle identifier")
  set(MACOSX_BUNDLE_BUNDLE_NAME "CrossPoint")
  set(MACOSX_BUNDLE_SHORT_VERSION_STRING "1.0")
  set(MACOSX_BUNDLE_BUNDLE_VERSION "1")
  configure_file("${CMAKE_CURRENT_SOURCE_DIR}/Info.plist.in" "${CMAKE_BINARY_DIR}/Info.plist" @ONLY)

  set_target_properties(crosspoint_simulator PROPERTIES
    MACOSX_BUNDLE_INFO_PLIST "${CMAKE_BINARY_DIR}/Info.plist"
    XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "${MACOSX_BUNDLE_GUI_IDENTIFIER}"
    XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1,2"        # iPhone and iPad
    # 14.0 covers the UTType and folder picker APIs this app calls directly.
    # 26.0 is forced by the firmware: CssParser uses the floating-point
    # std::from_chars, which lives in the libc++ dylib and is annotated
    # unavailable before iOS 26.
    # ponytail: raising the floor is one line against rewriting a hot parser.
    # If this has to run on an older iPhone, give tryParseNumber a bounded
    # strtod path for floating-point T instead -- from_chars takes an end
    # pointer, strtod does not, so it needs a stack copy of the token.
    XCODE_ATTRIBUTE_IPHONEOS_DEPLOYMENT_TARGET "26.0"
    XCODE_ATTRIBUTE_ENABLE_BITCODE "NO"
    XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC "YES"
    XCODE_ATTRIBUTE_SWIFT_OBJC_BRIDGING_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/Simulator-Bridging-Header.h"
    XCODE_ATTRIBUTE_SWIFT_VERSION "5.0"
  )
endif()
