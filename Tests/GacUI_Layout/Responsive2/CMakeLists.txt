project(Responsive2)
add_executable(Responsive2
    ../../../Release/Tutorial/GacUI_Layout/Responsive2/UI/Source/DemoPartialClasses.cpp
    ../../../Release/Tutorial/GacUI_Layout/Responsive2/Main.cpp
    ../../App.cpp)
target_link_libraries(Responsive2 ${wGac_LIBRARIES})
