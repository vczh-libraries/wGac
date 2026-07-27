cmake_minimum_required(VERSION 3.20)
project(WGacFullControlTest LANGUAGES CXX)

file(GLOB FULL_CONTROL_TEST_GENERATED_SOURCES
    CONFIGURE_DEPENDS
    "../Apps/FullControlTest/Source/*.cpp"
)
list(FILTER FULL_CONTROL_TEST_GENERATED_SOURCES EXCLUDE REGEX "Reflection\\.cpp$")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "bin/")

add_executable(Test_FullControlTest
    "Main.cpp"
    ${FULL_CONTROL_TEST_GENERATED_SOURCES}
)
target_include_directories(Test_FullControlTest PRIVATE
    "../Import"
    "../WGac"
    "../WGacShared"
    "../Apps/FullControlTest/Source"
)
target_link_libraries(Test_FullControlTest PRIVATE
    WGac
    WGacShared
)
