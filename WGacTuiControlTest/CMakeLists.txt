set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "bin/")

add_library(WGacTui STATIC
    "../WGac/TUI/TuiWGacController.cpp"
)
target_link_libraries(WGacTui PUBLIC WGac)

add_executable(Test_TuiControlTest
    "Main.cpp"
    "GuiMain.cpp"
    "../Apps/TuiControlTest/Source/TuiControlTestEventHandlers.cpp"
    "../Apps/TuiControlTest/Source/SharedDocumentEditor.cpp"
    "../Apps/TuiControlTest/Source/TuiControlTestPartialClasses.cpp"
    "../Apps/TuiControlTest/Source/TuiControlTestResource.cpp"
    "../Import/Skins/TuiSkin/TuiSkin.cpp"
)
target_include_directories(Test_TuiControlTest PRIVATE
    "../Apps/TuiControlTest/Source"
)
target_link_libraries(Test_TuiControlTest PRIVATE WGacTui)
