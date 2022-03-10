cmake_minimum_required(VERSION 3.15)
project(reddit_desktop_tests)

find_package(Threads)
find_package(GTest CONFIG REQUIRED)

include_directories(${CMAKE_SOURCE_DIR}/src
    "${CMAKE_CURRENT_SOURCE_DIR}/../external/url/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/../external/gumbo-parser/src/"
        )
if(VCPKG_TOOLCHAIN)
    include_directories("${CMAKE_SOURCE_DIR}/src/strings")
endif()

file(GLOB_RECURSE GUMBO_PARSER_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/../external/gumbo-parser/src/*.c")


set(BOOST_URL_SRC ../external/url/src/src.cpp)


set(RD_TEST_SRCS
    test_utils.cpp
    test_htmlparser.cpp
    )

set(TESTING_RD_CLASSES
    ${CMAKE_SOURCE_DIR}/src/htmlparser.cpp    
    )

add_executable(${PROJECT_NAME} ${BOOST_URL_SRC}
    ${RD_TEST_SRCS}
    ${TESTING_RD_CLASSES}
    ${GUMBO_PARSER_SRCS}
    )

target_link_libraries(${PROJECT_NAME} PRIVATE GTest::gtest GTest::gtest_main)
target_link_libraries(${PROJECT_NAME} PRIVATE Boost::boost Boost::system Boost::filesystem)
target_link_libraries(${PROJECT_NAME} PRIVATE Threads::Threads OpenGL::GL )
target_compile_definitions(${PROJECT_NAME} PRIVATE "RDTEST_DATA_FOLDER=\"${CMAKE_CURRENT_SOURCE_DIR}/data\"")
