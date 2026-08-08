cmake_minimum_required(VERSION 3.20)
project(WGacCppTestRvm LANGUAGES CXX)

file(GLOB REMOTE_VIEW_MODEL_TEST_GENERATED_SOURCES
    CONFIGURE_DEPENDS
    "../Apps/RemoteViewModelTest/Source/*.cpp"
)
list(FILTER REMOTE_VIEW_MODEL_TEST_GENERATED_SOURCES EXCLUDE REGEX "Reflection\\.cpp$")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "bin/")

add_executable(Test_CppTest_Rvm
    "GuiMain.cpp"
    "Main.cpp"
    ${REMOTE_VIEW_MODEL_TEST_GENERATED_SOURCES}
)
target_include_directories(Test_CppTest_Rvm PRIVATE
    "../Import"
    "../Import-Test"
    "../WGac"
    "../WGacShared"
    "../Apps/RemoteViewModelTest/Source"
)
target_link_libraries(Test_CppTest_Rvm PRIVATE
    WGac
    WGacShared
)
