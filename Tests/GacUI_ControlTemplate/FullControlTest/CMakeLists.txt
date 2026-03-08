project(FullControlTest)
add_executable(FullControlTest
    ../../../Release/Tutorial/GacUI_ControlTemplate/BlackSkin/UI/FullControlTest/Source/DemoPartialClasses.cpp
    ../../../Release/Tutorial/GacUI_ControlTemplate/BlackSkin/UI/FullControlTest/Source/DocumentEditorBase.cpp
    ../../../Release/Tutorial/GacUI_ControlTemplate/BlackSkin/UI/FullControlTest/Source/MainWindow.cpp
    ../../../Release/Tutorial/GacUI_ControlTemplate/BlackSkin/Main.cpp
    ../../App.cpp)
target_include_directories(FullControlTest PRIVATE
    ../../../Release/Tutorial/GacUI_ControlTemplate/BlackSkin)
target_link_libraries(FullControlTest ${wGac_LIBRARIES})
