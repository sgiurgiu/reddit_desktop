
file(GLOB_RECURSE HEADERS *.h *.hpp *.hxx )
add_custom_target(Headers SOURCES ${HEADERS})

SET(IMGUI_SRCS
    imgui/imgui.cpp
    imgui/imgui_demo.cpp
    imgui/imgui_draw.cpp
    imgui/imgui_impl_sdl.cpp
    imgui/imgui_impl_opengl3.cpp
    imgui/imgui_widgets.cpp
    imgui/imgui_stdlib.cpp
    spinner/spinner.cpp)

SET(GLAD_SRCS glad/glad.c)

SET(FMT_SRCS ../external/fmt/src/format.cc)

set(BOOST_URL_SRC ../external/url/src/src.cpp)

set(SQLITE_SRC sqlite/sqlite3.c)

set(REDDIT_DESKTOP_SRCS main.cpp
        utils.cpp
        redditdesktop.cpp
        subredditwindow.cpp
        database.cpp
        loginwindow.cpp
        redditclient.cpp
        redditloginconnection.cpp
        redditlistingconnection.cpp
        entities.cpp
        redditresourceconnection.cpp
        commentswindow.cpp
        markdownrenderer.cpp
        mediastreamingconnection.cpp
        )

add_executable(${EXECUTABLE_NAME}
    ${WIN32_APP_OPTIONS}
    ${REDDIT_DESKTOP_SRCS}
    ${IMGUI_SRCS}
    ${GLAD_SRCS}
    ${FMT_SRCS}
    ${SQLITE_SRC}
    ${BOOST_URL_SRC}
    )

target_link_libraries(${EXECUTABLE_NAME} PRIVATE  OpenGL::GL SDL2::SDL2)
target_link_libraries(${EXECUTABLE_NAME} PRIVATE ${CMAKE_DL_LIBS} Threads::Threads )
target_link_libraries(${EXECUTABLE_NAME} PRIVATE ${Boost_LIBRARIES} OpenSSL::SSL)
target_link_libraries(${EXECUTABLE_NAME} PRIVATE libcmark-gfm-extensions_static libcmark-gfm_static)
target_link_libraries(${EXECUTABLE_NAME} PRIVATE PkgConfig::FFMPEG)
target_include_directories(${EXECUTABLE_NAME} PRIVATE
    "imgui"
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/../external/fmt/include/"
    "${CMAKE_CURRENT_SOURCE_DIR}/../external/stb/"
    "${CMAKE_CURRENT_SOURCE_DIR}/../external/url/include"
    ${Boost_INCLUDE_DIRS}
    "${CMAKE_CURRENT_SOURCE_DIR}/../external/cmark-gfm/src"
    "${CMAKE_CURRENT_SOURCE_DIR}/../external/cmark-gfm/extensions"
    "${CMAKE_BINARY_DIR}/external/cmark-gfm/src"
    "${CMAKE_BINARY_DIR}/external/cmark-gfm/extensions"
    )

target_compile_definitions(${EXECUTABLE_NAME} PRIVATE IMGUI_IMPL_OPENGL_LOADER_GLAD BOOST_ASIO_NO_DEPRECATED)
target_compile_definitions(${EXECUTABLE_NAME} PRIVATE "$<$<CONFIG:Debug>:REDDIT_DESKTOP_DEBUG>")
target_compile_options(${EXECUTABLE_NAME} PUBLIC $<$<CONFIG:Debug>:-Wall -pedantic -Wextra>)
add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")
target_compile_definitions(${EXECUTABLE_NAME} PRIVATE "$<$<C_COMPILER_ID:MSVC>:_CRT_SECURE_NO_WARNINGS>")
target_compile_definitions(${EXECUTABLE_NAME} PRIVATE "$<$<C_COMPILER_ID:MSVC>:NOMINMAX>")
target_compile_definitions(${EXECUTABLE_NAME} PRIVATE "$<$<C_COMPILER_ID:MSVC>:_UNICODE>")
target_compile_definitions(${EXECUTABLE_NAME} PRIVATE "$<$<C_COMPILER_ID:MSVC>:UNICODE>")

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/../test/spec.txt" "${CMAKE_CURRENT_BINARY_DIR}/spec.txt" COPYONLY)


if(WIN32_APP_OPTIONS)
    target_compile_definitions(${EXECUTABLE_NAME} PRIVATE WIN32_WINMAIN)
endif()

install(TARGETS ${EXECUTABLE_NAME} DESTINATION bin)
