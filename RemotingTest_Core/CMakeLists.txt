set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "bin/")

file(GLOB CORE_RESOURCE_SOURCES CONFIGURE_DEPENDS
    "../Apps/FullControlTest/Source/*.cpp"
    "../Apps/RemoteProtocolTest/Source/*.cpp"
    "../Apps/RemoteViewModelTest/Source/*.cpp"
)
list(FILTER CORE_RESOURCE_SOURCES EXCLUDE REGEX "Reflection\\.cpp$")

add_executable(RemotingTest_Core Main.cpp GuiMain.cpp ${CORE_RESOURCE_SOURCES})
target_compile_definitions(RemotingTest_Core PRIVATE VCZH_DEBUG_NO_REFLECTION)
target_include_directories(RemotingTest_Core PRIVATE
    "../Import"
    "../Import-Test"
    "../Apps/FullControlTest/Source"
    "../Apps/RemoteProtocolTest/Source"
    "../Apps/RemoteViewModelTest/Source"
)
target_link_libraries(RemotingTest_Core PRIVATE WGacShared WGac)
