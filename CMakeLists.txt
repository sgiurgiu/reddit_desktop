cmake_minimum_required(VERSION 3.15)
project(reddit_desktop LANGUAGES C CXX
    VERSION 0.0.1
    DESCRIPTION "Reddit desktop"
    )

include("${CMAKE_SOURCE_DIR}/cmake/CMakeCCache.txt")

option(ENABLE_TESTS "Build tests" ON)
option(ASAN_ENABLE "Build with sanitization on" OFF)
option(ENABLE_CMARK "Use cmark as the markdown parser" OFF)
option(ENABLE_M4DC "Use m4dc as the markdown parser" ON)

if(ENABLE_M4DC AND ENABLE_CMARK)
    # TODO: Make this a runtime choice not a compile time one
    message(ERROR "Please only choose one of cmark or m4dc markdown parsers")
endif()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

if(DEFINED ENV{REDDITDESKTOP_VERSION_MAJOR})
    SET(REDDITDESKTOP_VERSION_MAJOR "$ENV{REDDITDESKTOP_VERSION_MAJOR}")
endif()
if(DEFINED ENV{REDDITDESKTOP_VERSION_MINOR})
    SET(REDDITDESKTOP_VERSION_MINOR "$ENV{REDDITDESKTOP_VERSION_MINOR}")
endif()
if(DEFINED ENV{REDDITDESKTOP_VERSION_PATCH})
    SET(REDDITDESKTOP_VERSION_PATCH "$ENV{REDDITDESKTOP_VERSION_PATCH}")
endif()

if(NOT REDDITDESKTOP_VERSION_MAJOR)
    SET(REDDITDESKTOP_VERSION_MAJOR "0")
endif()
if(NOT REDDITDESKTOP_VERSION_MINOR)
    SET(REDDITDESKTOP_VERSION_MINOR "0")
endif()
if(NOT REDDITDESKTOP_VERSION_PATCH)
    SET(REDDITDESKTOP_VERSION_PATCH "1")
endif()

add_definitions(-DREDDITDESKTOP_VERSION="${REDDITDESKTOP_VERSION_MAJOR}.${REDDITDESKTOP_VERSION_MINOR}.${REDDITDESKTOP_VERSION_PATCH}")
add_definitions(-DREDDITDESKTOP_STRING="Reddit Desktop v${REDDITDESKTOP_VERSION_MAJOR}.${REDDITDESKTOP_VERSION_MINOR}.${REDDITDESKTOP_VERSION_PATCH}")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

if(MSVC)
    set(WIN32_APP_OPTIONS WIN32)
endif()
set(Boost_USE_STATIC_LIBS        ON)
set(Boost_USE_MULTITHREADED      ON)
if(VCPKG_TOOLCHAIN)
set(Boost_USE_STATIC_RUNTIME ON)
endif()

set(OpenGL_GL_PREFERENCE GLVND)
find_package(OpenGL COMPONENTS OpenGL REQUIRED)
find_package(Threads REQUIRED)
find_package(SDL2 CONFIG REQUIRED)
find_package(Boost 1.74 REQUIRED system filesystem program_options)
find_package(OpenSSL REQUIRED)

if(VCPKG_TOOLCHAIN)
    find_library(LIBMPV mpv ${LIBMPV_DIR})
    find_package(freetype CONFIG REQUIRED)
    #ADD IBUS TO VCPKG
else()
    #set(ENV{PKG_CONFIG_PATH} "/usr/local/lib/pkgconfig:$ENV{PKG_CONFIG_PATH}")
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(MPV REQUIRED IMPORTED_TARGET mpv)
    pkg_check_modules(FREETYPE REQUIRED IMPORTED_TARGET freetype2)
    pkg_check_modules(IBUS REQUIRED IMPORTED_TARGET ibus-1.0)
endif()


if(ENABLE_CMARK)
    set(CMARK_STATIC ON)
    set(CMARK_SHARED OFF)
    set(CMARK_TESTS OFF)
    add_subdirectory(external/cmark-gfm)
endif()
add_subdirectory(external/spdlog)
add_subdirectory(external/fmt)

if(UNIX AND NOT APPLE)
    set(LINUX TRUE) #wrong here, what if we're BSD?
endif()

if(LINUX)
    add_definitions(-DRD_LINUX)
endif()
if(WIN32)
    add_definitions(-DRD_WINDOWS)
endif()
if(APPLE)
    add_definitions(-DRD_APPLE)
endif()

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

set(EXECUTABLE_NAME reddit_desktop)

add_subdirectory(src)

if(ENABLE_TESTS)
    add_subdirectory(test)
endif()

SET(CPACK_PACKAGE_NAME "reddit_desktop")
SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Reddit Resktop Application")
SET(CPACK_RESOURCE_FILE_LICENSE_PROVIDED FALSE)
SET(CPACK_INTERACTIVE FALSE)
SET(CPACK_INCLUDE_SUBDIR TRUE)
SET(CPACK_PACKAGE_VERSION_MAJOR ${REDDITDESKTOP_VERSION_MAJOR})
SET(CPACK_PACKAGE_VERSION_MINOR ${REDDITDESKTOP_VERSION_MINOR})
SET(CPACK_PACKAGE_VERSION_PATCH ${REDDITDESKTOP_VERSION_PATCH})
SET(CPACK_PACKAGE_CONTACT "Sergiu Giurgiu <sgiurgiu11@gmail.com>")
set (CPACK_DEBIAN_PACKAGE_DEPENDS "libmpv1, libssl1.1, libfreetype6")

set(CPACK_RPM_RELOCATION_PATHS "/${CMAKE_INSTALL_SYSCONFDIR}" "${CMAKE_INSTALL_BINDIR}")
set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY 0) # don't prepend package name
#set(CPACK_PACKAGING_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX})
#set(CPACK_RPM_POST_INSTALL_SCRIPT_FILE "${CMAKE_CURRENT_SOURCE_DIR}/postinstall")

SET(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}")

if(CPACK_DISTRIBUTION)
    SET(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_FILE_NAME}-${CPACK_DISTRIBUTION}")
endif()

install(FILES "${CMAKE_SOURCE_DIR}/icons/reddit_icon_24.png"  DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/24x24/apps" RENAME "reddit-desktop.png")
install(FILES "${CMAKE_SOURCE_DIR}/icons/reddit_icon_32.png"  DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/32x32/apps" RENAME "reddit-desktop.png")
install(FILES "${CMAKE_SOURCE_DIR}/icons/reddit_icon_48.png"  DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/48x48/apps" RENAME "reddit-desktop.png")
install(FILES "${CMAKE_SOURCE_DIR}/icons/reddit_icon_64.png"  DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/64x64/apps" RENAME "reddit-desktop.png")
install(FILES "${CMAKE_SOURCE_DIR}/icons/reddit_icon_128.png" DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/128x128/apps" RENAME "reddit-desktop.png")
install(FILES "${CMAKE_SOURCE_DIR}/icons/reddit_icon_256.png" DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/256x256/apps" RENAME "reddit-desktop.png")
install(FILES "${CMAKE_SOURCE_DIR}/desktop/reddit_desktop.desktop" DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/applications" RENAME "reddit-desktop.desktop")

set(CPACK_PACKAGE_EXECUTABLES "${EXECUTABLE_NAME};Reddit Desktop")
if(CPACK_GENERATOR STREQUAL "WIX")
    set(CPACK_WIX_UPGRADE_GUID "5108A593-CBD7-428C-84BA-FF202D3AD299")
    set(CPACK_WIX_PRODUCT_ICON "${CMAKE_SOURCE_DIR}/icons/reddit_icon_48.png")
    set(CPACK_PACKAGE_ICON "${CMAKE_SOURCE_DIR}/icons/reddit_icon_48.png")
    set(CPACK_WIX_PROGRAM_MENU_FOLDER "Reddit Desktop")
    set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/gpl-3.0.rtf")
endif()

INCLUDE(CPack)
