cmake_minimum_required(VERSION 3.20)
project(WGacTest LANGUAGES CXX)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "bin/")

add_executable(Test_HellWorld_Cpp
    "HelloWorlds/Cpp/Main.cpp"
)
target_include_directories(Test_HellWorld_Cpp PRIVATE
    "../Import"
    "../WGac"
    "../WGacShared"
)
target_link_libraries(Test_HellWorld_Cpp PRIVATE
    WGac
    WGacShared
)

add_executable(Test_AsyncService "AsyncService.cpp")
target_link_libraries(Test_AsyncService PRIVATE WGac)

add_executable(Test_ImageService "ImageService.cpp")
target_link_libraries(Test_ImageService PRIVATE WGac)

add_executable(Test_GlobalShortcuts "GlobalShortcuts.cpp")
target_link_libraries(Test_GlobalShortcuts PRIVATE WGac)

add_executable(Test_RenderingLifecycle "RenderingLifecycle.cpp")
target_link_libraries(Test_RenderingLifecycle PRIVATE WGac)

add_executable(Test_RawElement
    "RawElement.cpp"
    "../WGacShared/FullControlTestRawElement.cpp"
    "../WGacShared/FullControlTestRawElementRemote.cpp"
)
target_link_libraries(Test_RawElement PRIVATE WGac)
