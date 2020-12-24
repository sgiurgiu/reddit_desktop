cmake_minimum_required(VERSION 3.15)
project(reddit_desktop_tests)

configure_file(${CMAKE_SOURCE_DIR}/cmake/GoogleTest-CMakeLists.txt.in ${CMAKE_BINARY_DIR}/googletest-download/CMakeLists.txt)
execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
  RESULT_VARIABLE result
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/googletest-download )
if(result)
  message(FATAL_ERROR "CMake step for googletest failed: ${result}")
endif()
execute_process(COMMAND ${CMAKE_COMMAND} --build .
  RESULT_VARIABLE result
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/googletest-download )
if(result)
  message(FATAL_ERROR "Build step for googletest failed: ${result}")
endif()

set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Add googletest directly to our build. This defines
# the gtest and gtest_main targets.
add_subdirectory(${CMAKE_BINARY_DIR}/googletest-src
                 ${CMAKE_BINARY_DIR}/googletest-build
                 EXCLUDE_FROM_ALL)

find_package(Threads)

include_directories(${CMAKE_SOURCE_DIR}/src
    "${CMAKE_CURRENT_SOURCE_DIR}/../external/url/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/../external/gumbo-parser/src/"
        )

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


target_link_libraries(${PROJECT_NAME} PRIVATE gtest_main Threads::Threads OpenGL::GL )
target_compile_definitions(${PROJECT_NAME} PRIVATE "RDTEST_DATA_FOLDER=\"${CMAKE_CURRENT_SOURCE_DIR}/data\"")
