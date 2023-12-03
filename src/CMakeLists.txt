file(GLOB_RECURSE HEADERS *.h *.hpp *.hxx )
add_custom_target(Headers SOURCES ${HEADERS})


SET(IMGUI_SRCS
    imgui/imgui.cpp
    imgui/imgui_demo.cpp
    imgui/imgui_draw.cpp
    imgui/imgui_impl_glfw.cpp
    imgui/imgui_impl_opengl3.cpp
    imgui/imgui_widgets.cpp
    imgui/imgui_stdlib.cpp
    imgui/imgui_tables.cpp
    imgui/misc/freetype/imgui_freetype.cpp
    spinner/spinner.cpp)

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
    connections/urldetectionconnection.cpp
    connections/redditcreatepostconnection.cpp
    connections/redditsearchnamesconnection.cpp
    connections/redditvoteconnection.cpp
    connections/redditmorechildrenconnection.cpp
    connections/redditcommentconnection.cpp
    connections/redditmarkreplyreadconnection.cpp
    connections/redditsrsubscriptionconnection.cpp
    connections/redditlivethreadconnection.cpp
    connections/twitterconnection.cpp
    connections/ipinfoconnection.cpp
    connections/proxy/socks5.cpp
    connections/proxy/socks4.cpp
    connections/proxy/proxy.cpp
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
        aboutwindow.cpp
        awardsrenderer.cpp
        globalresourcescache.cpp
        flairrenderer.cpp
        livethreadviewer.cpp
        twitterrenderer.cpp
        uri.cpp
        RDRect.cpp
        networkproxysettingswindow.cpp
        )

if(WIN32)
    list(APPEND REDDIT_DESKTOP_SRCS win/getopt.c)
endif()

if(MSVC)
    list(APPEND REDDIT_DESKTOP_SRCS win/AppIcon.rc)
endif()

set(EXECUTABLE_SOURCES
    ${WIN32_APP_OPTIONS}
    ${REDDIT_DESKTOP_SRCS}
    ${IMGUI_SRCS}
    ${MARKDOWN_RENDERER_SRCS}
    ${REDDIT_CONNECTIONS_SRCS}
    ${REDDIT_DESKTOP_LOG_SRCS}
)

if(ENABLE_M4DC)
    list(APPEND EXECUTABLE_SOURCES  ${MD4C_SRC})
endif()


add_executable(${EXECUTABLE_NAME} ${EXECUTABLE_SOURCES})

set_property(TARGET ${EXECUTABLE_NAME} PROPERTY POSITION_INDEPENDENT_CODE TRUE)

target_link_libraries(${EXECUTABLE_NAME} PRIVATE  ${RD_LIBARIES})
target_include_directories(${EXECUTABLE_NAME} PRIVATE ${RD_INCLUDES})


set(EXECUTABLE_INCLUDES
    "imgui"
    "${Stb_INCLUDE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}"
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
target_compile_definitions(${EXECUTABLE_NAME} PRIVATE GL_GLEXT_PROTOTYPES BOOST_ASIO_NO_DEPRECATED)
target_compile_definitions(${EXECUTABLE_NAME} PRIVATE IMGUI_DEFINE_MATH_OPERATORS)
target_compile_definitions(${EXECUTABLE_NAME} PRIVATE "$<$<CONFIG:Debug>:REDDIT_DESKTOP_DEBUG>")
if(ASAN_ENABLE)
    target_compile_options(${EXECUTABLE_NAME} PUBLIC $<$<AND:$<CONFIG:Debug>,$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>>:-Wall -pedantic -Wextra -fsanitize=undefined,address -fno-omit-frame-pointer>)
    target_link_options(${EXECUTABLE_NAME} PUBLIC $<$<AND:$<CONFIG:Debug>,$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>>:-fsanitize=undefined,address>)
else()
    target_compile_options(${EXECUTABLE_NAME} PUBLIC $<$<AND:$<CONFIG:Debug>,$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>>:-Wall -pedantic -Wextra>)
endif(ASAN_ENABLE)

if(PACKAGING)
    target_compile_definitions(${EXECUTABLE_NAME} PRIVATE "FONTS_DIRECTORY=\"${CMAKE_INSTALL_PREFIX}/${FONTS_DIRECTORY}\"")
endif()

target_compile_definitions(${EXECUTABLE_NAME} PRIVATE "$<$<C_COMPILER_ID:MSVC>:_CRT_SECURE_NO_WARNINGS>")
target_compile_definitions(${EXECUTABLE_NAME} PRIVATE "$<$<C_COMPILER_ID:MSVC>:NOMINMAX>")
# Windows 7 target
target_compile_definitions(${EXECUTABLE_NAME} PRIVATE "$<$<C_COMPILER_ID:MSVC>:_WIN32_WINNT=0x0601>")
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

install(TARGETS ${EXECUTABLE_NAME} DESTINATION ${CMAKE_INSTALL_BINDIR})


if(WIN32)
    file(REAL_PATH ${LIBMPV_DIR} MPV_DIR)    
    install(FILES "${MPV_DIR}/mpv-2.dll"  DESTINATION ${CMAKE_INSTALL_BINDIR})
    if(YOUTUBE_DL)
        file(REAL_PATH "${YOUTUBE_DL}" YOUTUBE_DL_EXE)
        install(PROGRAMS "${YOUTUBE_DL_EXE}" DESTINATION ${CMAKE_INSTALL_BINDIR})
    endif()
endif()

