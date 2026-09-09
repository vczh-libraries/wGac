set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "bin/")

find_package(X11 REQUIRED COMPONENTS Xfixes)

add_library(WGacTui STATIC
    "../WGac/TUI/TuiWGacController.cpp"
    "../WGac/TUI/TuiWGacClipboardService.cpp"
)
target_link_libraries(WGacTui PUBLIC WGac X11::X11 X11::Xfixes)

add_executable(Test_TuiControlTest
    "Main.cpp"
    "GuiMain.cpp"
    "../Apps/TuiControlTest/Source/TuiControlTestPartialClasses.cpp"
    "../Apps/TuiControlTest/Source/TuiControlTestResource.cpp"
    "../Import/Skins/TuiSkin/TuiSkin.cpp"
)
target_include_directories(Test_TuiControlTest PRIVATE
    "../Apps/TuiControlTest/Source"
)
target_link_libraries(Test_TuiControlTest PRIVATE WGacTui)
