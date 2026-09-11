set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "bin/")

# FindX11 keeps successful searches cached even after a temporary sysroot is
# removed. Rediscover missing paths before creating its imported targets.
get_cmake_property(WGAC_X11_CACHE_VARIABLES CACHE_VARIABLES)
foreach(WGAC_X11_CACHE_VARIABLE IN LISTS WGAC_X11_CACHE_VARIABLES)
    if(WGAC_X11_CACHE_VARIABLE MATCHES "^X11_.*(_INCLUDE_PATH|_LIB)$")
        if(IS_ABSOLUTE "${${WGAC_X11_CACHE_VARIABLE}}" AND NOT EXISTS "${${WGAC_X11_CACHE_VARIABLE}}")
            unset("${WGAC_X11_CACHE_VARIABLE}" CACHE)
        endif()
    endif()
endforeach()

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
