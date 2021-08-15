file(GLOB_RECURSE HEADERS *.h *.hpp *.hxx )
add_custom_target(Headers SOURCES ${HEADERS})

file(GLOB_RECURSE FONTS "${CMAKE_CURRENT_SOURCE_DIR}/../fonts/*.ttf")
message("Fonts collected: ${FONTS}")

SET(IMGUI_SRCS
    imgui/imgui.cpp
    imgui/imgui_demo.cpp
    imgui/imgui_draw.cpp
    imgui/imgui_impl_sdl.cpp
    imgui/imgui_impl_opengl3.cpp
    imgui/imgui_widgets.cpp
    imgui/imgui_stdlib.cpp
    imgui/imgui_tables.cpp
    imgui/misc/freetype/imgui_freetype.cpp
    spinner/spinner.cpp)

SET(GLAD_SRCS glad/glad.c)

set(BOOST_URL_SRC ../external/url/src/src.cpp)

set(SQLITE_SRC sqlite/sqlite3.c)

file(GLOB_RECURSE GUMBO_PARSER_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/../external/gumbo-parser/src/*.c")

if(ENABLE_M4DC)
    set(MD4C_SRC ../external/md4c/src/md4c.c)
endif()

set (MARKDOWN_RENDERER_SRCS
    markdown/markdownnode.cpp
    markdown/markdownnodeblockquote.cpp
    markdown/markdownnodeblockunorderedlist.cpp
    markdown/markdownnodeblockorderedlist.cpp
    markdown/markdownnodeblocklistitem.cpp
    markdown/markdownnodethematicbreak.cpp
    markdown/markdownnodehead.cpp
    markdown/markdownnodeblockcode.cpp
    markdown/markdownnodeblockhtml.cpp
    markdown/markdownnodeblockparagraph.cpp
    markdown/markdownnodetable.cpp
    markdown/markdownnodetableheader.cpp
    markdown/markdownnodetablebody.cpp
    markdown/markdownnodetablerow.cpp
    markdown/markdownnodetablecellhead.cpp
    markdown/markdownnodetablecell.cpp
    markdown/markdownnodeemphasis.cpp
    markdown/markdownnodestrong.cpp
    markdown/markdownnodeunderline.cpp
    markdown/markdownnodelink.cpp
    markdown/markdownnodeimage.cpp
    markdown/markdownnodecode.cpp
    markdown/markdownnodestrike.cpp
    markdown/markdownnodebreak.cpp
    markdown/markdownnodesoftbreak.cpp
    markdown/markdownnodetexthtml.cpp
    markdown/markdownnodetextentity.cpp
    markdown/markdownnodetext.cpp
    markdown/markdownnodedocument.cpp
    markdown/markdownparser.cpp
    )

if(ENABLE_M4DC)
    list(APPEND MARKDOWN_RENDERER_SRCS  markdown/m4dcmarkdownparser.cpp)
endif()
if(ENABLE_CMARK)
    list(APPEND MARKDOWN_RENDERER_SRCS  markdown/cmarkmarkdownparser.cpp)
endif()

set(REDDIT_CONNECTIONS_SRCS
    connections/redditloginconnection.cpp
    connections/redditlistingconnection.cpp
    connections/redditresourceconnection.cpp
    connections/mediastreamingconnection.cpp
    connections/redditcreatepostconnection.cpp
    connections/redditsearchnamesconnection.cpp
    connections/redditvoteconnection.cpp
    connections/redditmorechildrenconnection.cpp
    connections/redditcreatecommentconnection.cpp
    connections/redditmarkreplyreadconnection.cpp
    )

set(REDDIT_DESKTOP_LOG_SRCS
        log/spdlogwindowsink.cpp
        log/loggingwindow.cpp
   )

set(REDDIT_DESKTOP_SRCS main.cpp
        utils.cpp
        redditdesktop.cpp
        subredditwindow.cpp
        database.cpp
        loginwindow.cpp
        redditclientproducer.cpp
        entities.cpp
        commentswindow.cpp
        markdownrenderer.cpp
        htmlparser.cpp
        postcontentviewer.cpp
        resizableglimage.cpp
        userinformationwindow.cpp
        resizableinputtextmultiline.cpp
        subredditslistwindow.cpp
        mediawidget.cpp
        subredditstylesheet.cpp
        )

if(VCPKG_TOOLCHAIN)
    list(APPEND REDDIT_DESKTOP_SRCS win/getopt.c)
endif()

set(EXECUTABLE_SOURCES
    ${WIN32_APP_OPTIONS}
    ${REDDIT_DESKTOP_SRCS}
    ${IMGUI_SRCS}
    ${GLAD_SRCS}
    ${SQLITE_SRC}
    ${BOOST_URL_SRC}
    ${GUMBO_PARSER_SRCS}
    ${MARKDOWN_RENDERER_SRCS}
    ${REDDIT_CONNECTIONS_SRCS}
    ${REDDIT_DESKTOP_LOG_SRCS}
)

if(ENABLE_M4DC)
    list(APPEND EXECUTABLE_SOURCES  ${MD4C_SRC})
endif()


add_executable(${EXECUTABLE_NAME} ${EXECUTABLE_SOURCES})

#Disable warnings for glad, gumbo, katana and sqlite, they're thousands and I really don't care about them
set_source_files_properties(${GLAD_SRCS}
    ${GUMBO_PARSER_SRCS} ${KATANA_PARSER_SRCS} ${SQLITE_SRC}
    PROPERTIES COMPILE_FLAGS "-w")

target_link_libraries(${EXECUTABLE_NAME} PRIVATE  OpenGL::GL SDL2::SDL2)
target_link_libraries(${EXECUTABLE_NAME} PRIVATE ${CMAKE_DL_LIBS} Threads::Threads )
target_link_libraries(${EXECUTABLE_NAME} PRIVATE OpenSSL::SSL)
target_link_libraries(${EXECUTABLE_NAME} PRIVATE Boost::boost Boost::system Boost::filesystem)
target_link_libraries(${EXECUTABLE_NAME} PRIVATE spdlog::spdlog_header_only fmt::fmt-header-only)

if(VCPKG_TOOLCHAIN)
    target_include_directories(${EXECUTABLE_NAME} PRIVATE "win")
    target_link_libraries(${EXECUTABLE_NAME} PRIVATE ${LIBMPV} "crypt32.lib" freetype)
    target_include_directories(${EXECUTABLE_NAME} PRIVATE ${LIBMPV_INCLUDE})
else()
    target_link_libraries(${EXECUTABLE_NAME} PRIVATE PkgConfig::MPV PkgConfig::FREETYPE)
endif()

if(ENABLE_CMARK)
    target_link_libraries(${EXECUTABLE_NAME} PRIVATE libcmark-gfm-extensions_static libcmark-gfm_static)
endif()

set(EXECUTABLE_INCLUDES
    "imgui"
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/../external/stb/"
    "${CMAKE_CURRENT_SOURCE_DIR}/../external/url/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/../external/gumbo-parser/src/"
    )

if(ENABLE_M4DC)
    list(APPEND EXECUTABLE_INCLUDES  "${CMAKE_CURRENT_SOURCE_DIR}/../external/md4c/src")   
endif()

if(ENABLE_CMARK)
    list(APPEND EXECUTABLE_INCLUDES
        "${CMAKE_CURRENT_SOURCE_DIR}/../external/cmark-gfm/src"
        "${CMAKE_CURRENT_SOURCE_DIR}/../external/cmark-gfm/extensions"
        "${CMAKE_BINARY_DIR}/external/cmark-gfm/src"
        "${CMAKE_BINARY_DIR}/external/cmark-gfm/extensions"
        )
endif()

target_include_directories(${EXECUTABLE_NAME} PRIVATE ${EXECUTABLE_INCLUDES})

if(ENABLE_M4DC)
    target_compile_definitions(${EXECUTABLE_NAME} PRIVATE M4DC_ENABLED)
endif()
if(ENABLE_CMARK)
    target_compile_definitions(${EXECUTABLE_NAME} PRIVATE CMARK_ENABLED)
endif()

target_compile_definitions(${EXECUTABLE_NAME} PRIVATE SPDLOG_FMT_EXTERNAL_HO SPDLOG_FMT_EXTERNAL)
target_compile_definitions(${EXECUTABLE_NAME} PRIVATE IMGUI_IMPL_OPENGL_LOADER_GLAD BOOST_ASIO_NO_DEPRECATED)
target_compile_definitions(${EXECUTABLE_NAME} PRIVATE "$<$<CONFIG:Debug>:REDDIT_DESKTOP_DEBUG>")
if(ASAN_ENABLE)
    target_compile_options(${EXECUTABLE_NAME} PUBLIC $<$<AND:$<CONFIG:Debug>,$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>>:-Wall -pedantic -Wextra -fsanitize=undefined,address -fno-omit-frame-pointer>)
    target_link_options(${EXECUTABLE_NAME} PUBLIC $<$<AND:$<CONFIG:Debug>,$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>>:-fsanitize=undefined,address>)
else()
    target_compile_options(${EXECUTABLE_NAME} PUBLIC $<$<AND:$<CONFIG:Debug>,$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>>:-Wall -pedantic -Wextra>)
endif(ASAN_ENABLE)


target_compile_definitions(${EXECUTABLE_NAME} PRIVATE "$<$<C_COMPILER_ID:MSVC>:_CRT_SECURE_NO_WARNINGS>")
target_compile_definitions(${EXECUTABLE_NAME} PRIVATE "$<$<C_COMPILER_ID:MSVC>:NOMINMAX>")
#target_compile_definitions(${EXECUTABLE_NAME} PRIVATE "$<$<C_COMPILER_ID:MSVC>:_UNICODE>")
#target_compile_definitions(${EXECUTABLE_NAME} PRIVATE "$<$<C_COMPILER_ID:MSVC>:UNICODE>")
# Windows 7 target
target_compile_definitions(${EXECUTABLE_NAME} PRIVATE "$<$<C_COMPILER_ID:MSVC>:_WIN32_WINNT=0x0601>")
#target_compile_options(${EXECUTABLE_NAME} PRIVATE "$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")
#target_compile_options(${EXECUTABLE_NAME} PRIVATE "$<$<C_COMPILER_ID:MSVC>:/utf-8>")
target_compile_options(${EXECUTABLE_NAME} PRIVATE "$<$<CXX_COMPILER_ID:MSVC>:/bigobj>")
target_compile_options(${EXECUTABLE_NAME} PRIVATE "$<$<C_COMPILER_ID:MSVC>:/bigobj>")
set_property(TARGET ${EXECUTABLE_NAME} PROPERTY  MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/../test/data/spec.txt" "${CMAKE_CURRENT_BINARY_DIR}/spec.txt" COPYONLY)


file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/fonts)
add_custom_target(copy-test-makefile ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/fonts)

foreach(FONT ${FONTS})
    message("Copying ${FONT} to ${CMAKE_CURRENT_BINARY_DIR}/fonts")
    configure_file(${FONT} ${CMAKE_CURRENT_BINARY_DIR}/fonts/ COPYONLY)
endforeach()


if(WIN32_APP_OPTIONS)
    target_compile_definitions(${EXECUTABLE_NAME} PRIVATE WIN32_WINMAIN)
endif()

install(TARGETS ${EXECUTABLE_NAME} DESTINATION bin)
#install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/fonts DESTINATION bin)
