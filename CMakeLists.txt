cmake_minimum_required(VERSION 3.18)

option(ENABLE_TESTS "Build tests" ON)
option(ASAN_ENABLE "Build with sanitization on" OFF)
option(ENABLE_CMARK "Use cmark as the markdown parser" OFF)
option(ENABLE_M4DC "Use m4dc as the markdown parser" ON)
option(PACKAGING "Configure for packaging" OFF)

if(ENABLE_M4DC AND ENABLE_CMARK)
    # TODO: Make this a runtime choice not a compile time one
    message(ERROR "Please only choose one of cmark or m4dc markdown parsers")
endif()
set(VCPKG_OVERLAY_PORTS "${CMAKE_CURRENT_LIST_DIR}/vcpkg-ports")

if(ENABLE_TESTS)
  list(APPEND VCPKG_MANIFEST_FEATURES "tests")
endif()

project(reddit_desktop LANGUAGES C CXX
    VERSION 0.0.1
    DESCRIPTION "Reddit desktop"
    )

include("${CMAKE_SOURCE_DIR}/cmake/CMakeCCache.txt")


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

if(UNIX AND NOT APPLE)
    set(LINUX TRUE)
endif()

if(MSVC)
    set(WIN32_APP_OPTIONS WIN32)
endif()

set(Boost_USE_MULTITHREADED      ON)
set(OpenGL_GL_PREFERENCE GLVND)

find_package(Threads REQUIRED)

set(RD_LIBARIES ${CMAKE_DL_LIBS} Threads::Threads)
set(RD_INCLUDES)

set(Boost_USE_STATIC_LIBS    ON)
# Boost doesn't provide a pkgconfig file, so we just have to have it installed
# and discoverable by cmake
find_package(Boost 1.74 REQUIRED system filesystem program_options)
list(APPEND RD_LIBARIES Boost::boost Boost::system Boost::filesystem)

if(VCPKG_TOOLCHAIN)
    set(Boost_USE_STATIC_RUNTIME ON)
    find_package(OpenGL COMPONENTS OpenGL REQUIRED)
    find_package(GLEW REQUIRED)
    find_package(glfw3 CONFIG REQUIRED)

    find_package(OpenSSL REQUIRED)
    find_package(spdlog CONFIG REQUIRED)
    find_package(fmt CONFIG REQUIRED)
    find_package(freetype CONFIG REQUIRED)
    find_library(GUMBO_LIB gumbo REQUIRED)
    find_package(unofficial-sqlite3 CONFIG REQUIRED)
    find_package(uriparser CONFIG REQUIRED)
    find_package(Stb REQUIRED)
    if(UNIX)
        find_package(PkgConfig REQUIRED)
        pkg_check_modules(MPV REQUIRED IMPORTED_TARGET mpv)
        list(APPEND RD_LIBARIES PkgConfig::MPV)
    endif()

    list(APPEND RD_LIBARIES OpenGL::GL glfw GLEW::GLEW freetype)
    list(APPEND RD_LIBARIES OpenSSL::SSL)
    list(APPEND RD_LIBARIES spdlog::spdlog_header_only fmt::fmt-header-only)
    list(APPEND RD_LIBARIES ${GUMBO_LIB})
    list(APPEND RD_LIBARIES unofficial::sqlite3::sqlite3)
    list(APPEND RD_LIBARIES uriparser::uriparser)
elseif(UNIX)
    # if no vcpkg, and on UNIX fallback on PkgConfig
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(MPV REQUIRED IMPORTED_TARGET mpv)
    pkg_check_modules(OpenGL REQUIRED IMPORTED_TARGET opengl)
    pkg_check_modules(Glew REQUIRED IMPORTED_TARGET glew)
    pkg_check_modules(glfw3 REQUIRED IMPORTED_TARGET glfw3)
    pkg_check_modules(OpenSSL REQUIRED IMPORTED_TARGET openssl)
    pkg_check_modules(SpdLog REQUIRED IMPORTED_TARGET spdlog)
    pkg_check_modules(FMT REQUIRED IMPORTED_TARGET fmt)
    pkg_check_modules(Freetype REQUIRED IMPORTED_TARGET freetype2)
    pkg_check_modules(Gumbo REQUIRED IMPORTED_TARGET gumbo)
    pkg_check_modules(Sqlite REQUIRED IMPORTED_TARGET sqlite3)
    pkg_check_modules(Uriparser REQUIRED IMPORTED_TARGET liburiparser)


    list(APPEND RD_LIBARIES PkgConfig::OpenGL PkgConfig::glfw3 PkgConfig::Glew PkgConfig::Freetype)
    list(APPEND RD_LIBARIES PkgConfig::OpenSSL)
    list(APPEND RD_LIBARIES PkgConfig::SpdLog PkgConfig::FMT)
    list(APPEND RD_LIBARIES PkgConfig::Gumbo)
    list(APPEND RD_LIBARIES PkgConfig::Sqlite)
    list(APPEND RD_LIBARIES PkgConfig::Uriparser)
    list(APPEND RD_LIBARIES PkgConfig::MPV)

    # Package STB doesn't seem to include any pc files on Linux (Fedora) nor on FreeBSD
    # just add the include dir. Can be defined by STB_INCLUDE_DIR parameter to cmake
    # otherwise just assume "/usr/local/include/stb"
    if(NOT STB_INCLUDE_DIR)
        set(STB_INCLUDE_DIR "/usr/local/include/stb")
    endif()
    list(APPEND RD_INCLUDES ${STB_INCLUDE_DIR})
endif()


if(WIN32)
    find_library(LIBMPV mpv ${LIBMPV_DIR})
    list(APPEND RD_LIBARIES ${LIBMPV} "crypt32.lib")
    list(APPEND RD_INCLUDES "win" ${LIBMPV_INCLUDE})
endif()



if(ENABLE_CMARK)
    set(CMARK_STATIC ON)
    set(CMARK_SHARED OFF)
    set(CMARK_TESTS OFF)
    add_subdirectory(external/cmark-gfm)
    list(APPEND RD_LIBARIES libcmark-gfm-extensions_static libcmark-gfm_static)
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
file(GLOB_RECURSE FONTS "${CMAKE_CURRENT_SOURCE_DIR}/fonts/*.ttf")
set(FONTS_DIRECTORY ${CMAKE_INSTALL_DATAROOTDIR}/reddit_desktop)

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
set(CPACK_RPM_PACKAGE_LICENSE "GPLv3")
SET(CPACK_PACKAGE_VENDOR "Sergiu Giurgiu <sgiurgiu11@gmail.com>")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libmpv1, libfreetype6, libopengl0")
set(CPACK_FREEBSD_PACKAGE_MAINTAINER "Sergiu Giurgiu <sgiurgiu11@gmail.com>")
set(CPACK_FREEBSD_PACKAGE_DEPS "multimedia/mpv" "security/openssl" "print/freetype2" "graphics/glew" "graphics/mesa-dri")
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
if(NOT WIN32)
install(FILES ${FONTS} DESTINATION ${FONTS_DIRECTORY}/fonts)
endif()

set(CPACK_PACKAGE_EXECUTABLES "${EXECUTABLE_NAME};Reddit Desktop")
if(CPACK_GENERATOR STREQUAL "WIX")
    set(CPACK_WIX_UPGRADE_GUID "5108A593-CBD7-428C-84BA-FF202D3AD299")
    set(CPACK_WIX_PRODUCT_ICON "${CMAKE_SOURCE_DIR}/icons/reddit_icon_48.png")
    set(CPACK_PACKAGE_ICON "${CMAKE_SOURCE_DIR}/icons/reddit_icon_48.png")
    set(CPACK_WIX_PROGRAM_MENU_FOLDER "Reddit Desktop")
    set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/gpl-3.0.rtf")
    set(CPACK_PACKAGE_INSTALL_DIRECTORY "Reddit Desktop")	
endif()



INCLUDE(CPack)
