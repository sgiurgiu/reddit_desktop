
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
        )

add_executable(${EXECUTABLE_NAME}
    ${WIN32_APP_OPTIONS}
    ${REDDIT_DESKTOP_SRCS}
    ${IMGUI_SRCS}
    ${GLAD_SRCS}
    ${FMT_SRCS}
    ${SQLITE_SRC}
    )

target_link_libraries(${EXECUTABLE_NAME} PRIVATE  OpenGL::GL SDL2::SDL2)
target_link_libraries(${EXECUTABLE_NAME} PRIVATE ${CMAKE_DL_LIBS} Threads::Threads )
target_link_libraries(${EXECUTABLE_NAME} PRIVATE ${Boost_LIBRARIES} OpenSSL::SSL)

target_include_directories(${EXECUTABLE_NAME} PRIVATE
    "imgui"
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/../external/fmt/include/"
    "${CMAKE_CURRENT_SOURCE_DIR}/../external/stb/"
    ${Boost_INCLUDE_DIRS}
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



if(WIN32_APP_OPTIONS)
    target_compile_definitions(${EXECUTABLE_NAME} PRIVATE WIN32_WINMAIN)
endif()

install(TARGETS ${EXECUTABLE_NAME} DESTINATION bin)
