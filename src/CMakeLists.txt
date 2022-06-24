file(GLOB_RECURSE HEADERS *.h *.hpp *.hxx )
add_custom_target(Headers SOURCES ${HEADERS})

if(ENABLE_M4DC)
    set(MD4C_SRC ../external/md4c/src/md4c.c)
endif()


set(REDDIT_DESKTOP_SRCS main.cpp
        entities.cpp
        database.cpp
        utils.cpp
        accesstokenprovider.cpp
        userinformationprovider.cpp
        entities/userinfo.cpp
        subscribedsubredditsprovider.cpp
        entities/subreddit.cpp
        subredditslistmodel.cpp
        postsprovider.cpp
        postslistmodel.cpp
        entities/post.cpp
        entities/image.cpp
        entities/media.cpp
        entities/postgalleryitem.cpp
        entities/award.cpp
        entities/flairrichtext.cpp
        )

if(MSVC)
    list(APPEND REDDIT_DESKTOP_SRCS win/AppIcon.rc)
endif()

set(EXECUTABLE_SOURCES
    ${WIN32_APP_OPTIONS}
    ${REDDIT_DESKTOP_SRCS}
)

if(ENABLE_M4DC)
    list(APPEND EXECUTABLE_SOURCES  ${MD4C_SRC})
endif()


qt_add_executable(${EXECUTABLE_NAME} ${EXECUTABLE_SOURCES})

set_property(TARGET ${EXECUTABLE_NAME} PROPERTY POSITION_INDEPENDENT_CODE TRUE)

target_link_libraries(${EXECUTABLE_NAME} PRIVATE ${CMAKE_DL_LIBS} Threads::Threads)
target_link_libraries(${EXECUTABLE_NAME} PRIVATE OpenSSL::SSL)
target_link_libraries(${EXECUTABLE_NAME} PRIVATE spdlog::spdlog_header_only fmt::fmt-header-only)
target_link_libraries(${EXECUTABLE_NAME} PRIVATE unofficial::sqlite3::sqlite3)

target_link_libraries(${EXECUTABLE_NAME} PRIVATE Qt6::Core Qt6::Gui Qt6::Qml
    Qt6::Quick Qt6::QuickControls2 Qt6::Network)
target_compile_definitions(${EXECUTABLE_NAME} PRIVATE $<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:QT_QML_DEBUG>)

set_source_files_properties(images/default.png PROPERTIES
    QT_RESOURCE_ALIAS qml/default.png
)
set_source_files_properties(images/image.png PROPERTIES
    QT_RESOURCE_ALIAS qml/image.png
)
set_source_files_properties(images/nsfw.png PROPERTIES
    QT_RESOURCE_ALIAS qml/nsfw.png
)
set_source_files_properties(images/reddit.png PROPERTIES
    QT_RESOURCE_ALIAS qml/reddit.png
)
set_source_files_properties(images/self.png PROPERTIES
    QT_RESOURCE_ALIAS qml/self.png
)

qt_add_qml_module(${EXECUTABLE_NAME}
    URI com.zergiu.reddit
    VERSION 1.0
    QML_FILES
        qml/main.qml
        qml/LoginDialog.qml
        qml/UserDrawer.qml
        qml/SubredditTabButton.qml
        qml/SubredditPage.qml
        qml/PostComponent.qml
    RESOURCES
        images/default.png
        images/image.png
        images/nsfw.png
        images/reddit.png
        images/self.png
)

qt_import_qml_plugins(${EXECUTABLE_NAME})

if(MSVC)
    target_include_directories(${EXECUTABLE_NAME} PRIVATE "win")
    target_link_libraries(${EXECUTABLE_NAME} PRIVATE ${LIBMPV} "crypt32.lib")
    target_include_directories(${EXECUTABLE_NAME} PRIVATE ${LIBMPV_INCLUDE})
else()
    target_link_libraries(${EXECUTABLE_NAME} PRIVATE PkgConfig::MPV)
#    target_link_libraries(${EXECUTABLE_NAME} PRIVATE PkgConfig::IBUS)
endif()

if(ENABLE_CMARK)
    target_link_libraries(${EXECUTABLE_NAME} PRIVATE libcmark-gfm-extensions_static libcmark-gfm_static)
endif()

set(EXECUTABLE_INCLUDES "${CMAKE_CURRENT_SOURCE_DIR}/entities")

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


install(TARGETS ${EXECUTABLE_NAME} DESTINATION ${CMAKE_INSTALL_BINDIR})

if(WIN32)
    file(REAL_PATH ${LIBMPV_DIR} MPV_DIR)    
    install(FILES "${MPV_DIR}/mpv-2.dll"  DESTINATION ${CMAKE_INSTALL_BINDIR})
    if(YOUTUBE_DL)
        file(REAL_PATH "${YOUTUBE_DL}" YOUTUBE_DL_EXE)
        install(PROGRAMS "${YOUTUBE_DL_EXE}" DESTINATION ${CMAKE_INSTALL_BINDIR})
    endif()
endif()

