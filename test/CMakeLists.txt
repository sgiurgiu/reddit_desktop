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


set(RD_TEST_SRCS
    test_utils.cpp
    test_htmlparser.cpp
    )


set(TESTING_RD_CLASSES
    ${CMAKE_SOURCE_DIR}/src/htmlparser.cpp
    ${CMAKE_SOURCE_DIR}/src/uri.cpp
    ${CMAKE_SOURCE_DIR}/src/entities.cpp
    ${CMAKE_SOURCE_DIR}/src/utils.cpp
    ${CMAKE_SOURCE_DIR}/src/resizableglimage.cpp
    )

add_executable(${PROJECT_NAME}

    )

target_link_libraries(${PROJECT_NAME} PRIVATE GTest::gtest GTest::gtest_main)
target_link_libraries(${PROJECT_NAME} PRIVATE Threads::Threads)
target_link_libraries(${PROJECT_NAME} PRIVATE OpenSSL::SSL)
target_compile_definitions(${PROJECT_NAME} PRIVATE "RDTEST_DATA_FOLDER=\"${CMAKE_CURRENT_SOURCE_DIR}/data\"")
