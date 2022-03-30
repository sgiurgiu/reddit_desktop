cmake_minimum_required(VERSION 3.15)
project(reddit_desktop_tests)

find_package(Threads)
find_package(GTest CONFIG REQUIRED)

include_directories(${CMAKE_SOURCE_DIR}/src    
    ${CMAKE_SOURCE_DIR}/src/imgui
        )
if(WIN32)
    include_directories("${CMAKE_SOURCE_DIR}/src/strings")
endif()

SET(IMGUI_SRCS
    ${CMAKE_SOURCE_DIR}/src/imgui/imgui.cpp
    ${CMAKE_SOURCE_DIR}/src/imgui/imgui_draw.cpp
    ${CMAKE_SOURCE_DIR}/src/imgui/imgui_widgets.cpp
    ${CMAKE_SOURCE_DIR}/src/imgui/imgui_stdlib.cpp
    ${CMAKE_SOURCE_DIR}/src/imgui/imgui_tables.cpp
    ${CMAKE_SOURCE_DIR}/src/imgui/misc/freetype/imgui_freetype.cpp
    )

set(RD_TEST_SRCS
    test_utils.cpp
    test_htmlparser.cpp
    )

set(REDDIT_CONNECTIONS_SRCS
    ${CMAKE_SOURCE_DIR}/src/connections/redditresourceconnection.cpp
    ${CMAKE_SOURCE_DIR}/src/connections/redditloginconnection.cpp
    ${CMAKE_SOURCE_DIR}/src/connections/redditlistingconnection.cpp
    ${CMAKE_SOURCE_DIR}/src/connections/urldetectionconnection.cpp
    ${CMAKE_SOURCE_DIR}/src/connections/redditcreatepostconnection.cpp
    ${CMAKE_SOURCE_DIR}/src/connections/redditsearchnamesconnection.cpp
    ${CMAKE_SOURCE_DIR}/src/connections/redditvoteconnection.cpp
    ${CMAKE_SOURCE_DIR}/src/connections/redditmorechildrenconnection.cpp
    ${CMAKE_SOURCE_DIR}/src/connections/redditcommentconnection.cpp
    ${CMAKE_SOURCE_DIR}/src/connections/redditmarkreplyreadconnection.cpp
    ${CMAKE_SOURCE_DIR}/src/connections/redditsrsubscriptionconnection.cpp
    ${CMAKE_SOURCE_DIR}/src/connections/redditlivethreadconnection.cpp
    ${CMAKE_SOURCE_DIR}/src/connections/twitterconnection.cpp
    ${CMAKE_SOURCE_DIR}/src/redditclientproducer.cpp
    )

set(TESTING_RD_CLASSES
    ${CMAKE_SOURCE_DIR}/src/htmlparser.cpp
    ${CMAKE_SOURCE_DIR}/src/uri.cpp
    ${CMAKE_SOURCE_DIR}/src/entities.cpp
    ${CMAKE_SOURCE_DIR}/src/utils.cpp
    ${CMAKE_SOURCE_DIR}/src/resizableglimage.cpp
    )

add_executable(${PROJECT_NAME}
    ${RD_TEST_SRCS}
    ${TESTING_RD_CLASSES}
    ${REDDIT_CONNECTIONS_SRCS}
    ${IMGUI_SRCS}
    )

target_link_libraries(${PROJECT_NAME} PRIVATE GTest::gtest GTest::gtest_main)
target_link_libraries(${PROJECT_NAME} PRIVATE Boost::boost Boost::system Boost::filesystem)
target_link_libraries(${PROJECT_NAME} PRIVATE Threads::Threads OpenGL::GL freetype)
target_link_libraries(${PROJECT_NAME} PRIVATE uriparser::uriparser)
target_link_libraries(${PROJECT_NAME} PRIVATE spdlog::spdlog_header_only fmt::fmt-header-only)
target_link_libraries(${PROJECT_NAME} PRIVATE ${GUMBO_LIB})
target_link_libraries(${PROJECT_NAME} PRIVATE OpenSSL::SSL)
target_compile_definitions(${PROJECT_NAME} PRIVATE "RDTEST_DATA_FOLDER=\"${CMAKE_CURRENT_SOURCE_DIR}/data\"")
