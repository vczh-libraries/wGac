cmake_minimum_required(VERSION 3.24)
project(wGac_Tests)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_FLAGS_DEBUG "-g -DDEBUG -gdwarf-4")
set(CMAKE_CXX_FLAGS_DEBUG "-g -DDEBUG -gdwarf-4")
add_compile_definitions(VCZH_DEBUG_NO_REFLECTION)

find_package(PkgConfig REQUIRED)
pkg_check_modules(WAYLAND REQUIRED wayland-client xkbcommon)
pkg_check_modules(CAIRO REQUIRED cairo pangocairo fontconfig)
pkg_check_modules(GDK_PIXBUF REQUIRED gdk-pixbuf-2.0)
pkg_check_modules(GIO REQUIRED gio-2.0)

# GacUI library
add_library(GacUI
    ../Release/Import/GacUI.cpp
    ../Release/Import/Vlpp.cpp
    ../Release/Import/Vlpp.Linux.cpp
    ../Release/Import/VlppOS.cpp
    ../Release/Import/VlppOS.Linux.cpp
    ../Release/Import/VlppRegex.cpp
    ../Release/Import/VlppReflection.cpp
    ../Release/Import/VlppGlrParser.cpp
    ../Release/Import/VlppWorkflowLibrary.cpp
    ../Release/Import/Skins/DarkSkin/DarkSkin.cpp
)
target_include_directories(GacUI PUBLIC ../Release/Import)

# wGac library
add_library(wGac
    ../Source/Services/WGacCallbackService.cpp
    ../Source/Services/WGacInputService.cpp
    ../Source/Services/WGacAsyncService.cpp
    ../Source/Services/WGacImageService.cpp
    ../Source/Services/WGacDialogService.cpp
    ../Source/Services/WGacResourceService.cpp
    ../Source/Services/WGacScreenService.cpp
    ../Source/Services/WGacClipboardService.cpp
    ../Source/Renderers/WGacRendererImpl.cpp
    ../Source/Wayland/WaylandDisplay.cpp
    ../Source/Wayland/WaylandBuffer.cpp
    ../Source/Wayland/WaylandSeat.cpp
    ../Source/WGacController.cpp
    ../Source/WGacNativeWindow.cpp
    ../Source/WGacGacView.cpp
    ../Source/WGacWindow.cpp
    ../Source/WGacWindowView.cpp
    ../Source/WGacCursor.cpp
    ../Source/Protocol/xdg-shell-protocol.c
    ../Source/Protocol/xdg-decoration-protocol.c
    ../Source/Protocol/text-input-unstable-v3-protocol.c
)
target_include_directories(wGac PUBLIC
    ../Source
    ../Source/Renderers
    ../Source/Services
    ../Source/Protocol
    ../Release/Import
    ${WAYLAND_INCLUDE_DIRS}
    ${CAIRO_INCLUDE_DIRS}
    ${GDK_PIXBUF_INCLUDE_DIRS}
)
target_link_libraries(wGac GacUI ${WAYLAND_LIBRARIES} ${CAIRO_LIBRARIES} ${GDK_PIXBUF_LIBRARIES} ${GIO_LIBRARIES})

list(APPEND wGac_INCLUDE_DIRS ../Release/Import ../Source ../Source/Renderers ../Source/Services)
list(APPEND wGac_LIBRARIES GacUI wGac)

include_directories(./ ${wGac_INCLUDE_DIRS})

# Test executables
add_subdirectory(GacUI_HelloWorlds/Cpp)
add_subdirectory(GacUI_HelloWorlds/CppXml)
add_subdirectory(GacUI_HelloWorlds/MVVM)
add_subdirectory(GacUI_Layout/Alignment)
add_subdirectory(GacUI_Layout/Flow)
add_subdirectory(GacUI_Layout/Responsive1)
add_subdirectory(GacUI_Layout/Responsive2)
add_subdirectory(GacUI_Layout/RichTextEmbedding)
add_subdirectory(GacUI_Layout/SharedSize)
add_subdirectory(GacUI_Layout/Stack)
add_subdirectory(GacUI_Layout/Table)
add_subdirectory(GacUI_Layout/TableSplitter)
add_subdirectory(GacUI_Xml/Binding_Bind)
add_subdirectory(GacUI_Xml/Binding_Eval)
add_subdirectory(GacUI_Xml/Binding_Format)
add_subdirectory(GacUI_Xml/Binding_Uri)
add_subdirectory(GacUI_Xml/Binding_ViewModel)
add_subdirectory(GacUI_Xml/Event_Cpp)
add_subdirectory(GacUI_Xml/Event_Script)
add_subdirectory(GacUI_Xml/Event_ViewModel)
add_subdirectory(GacUI_Xml/Instance_Control)
add_subdirectory(GacUI_Xml/Instance_MultipleWindows)
add_subdirectory(GacUI_Xml/Instance_Window)
add_subdirectory(GacUI_Xml/Member_Field)
add_subdirectory(GacUI_Xml/Member_Parameter)
add_subdirectory(GacUI_Xml/Member_Property)
add_subdirectory(GacUI_Xml/Misc_ImportFolder)
add_subdirectory(GacUI_Controls/AddressBook)
add_subdirectory(GacUI_Controls/Animation)
add_subdirectory(GacUI_Controls/CalculatorAndStateMachine)
add_subdirectory(GacUI_Controls/ColorPicker)
add_subdirectory(GacUI_Controls/ContainersAndButtons)
add_subdirectory(GacUI_Controls/DataGrid)
add_subdirectory(GacUI_Controls/DocumentEditorRibbon)
add_subdirectory(GacUI_Controls/DocumentEditorToolstrip)
add_subdirectory(GacUI_Controls/GlobalHotKey)
add_subdirectory(GacUI_Controls/ListControls)
add_subdirectory(GacUI_Controls/Localization)
add_subdirectory(GacUI_Controls/MenuVisibility)
add_subdirectory(GacUI_Controls/ProgressAndAsync)
add_subdirectory(GacUI_Controls/QueryService)
# add_subdirectory(GacUI_Controls/TextEditor)  # Disabled - API changes in 1.3.0
add_subdirectory(GacUI_Controls/TriplePhaseImageButton)
add_subdirectory(GacUI_ControlTemplate/WindowSkin)

# Copy resources
list(APPEND CATEGORIES GacUI_Controls GacUI_ControlTemplate GacUI_HelloWorlds GacUI_Layout GacUI_Xml)
foreach(CATEGORY IN LISTS CATEGORIES)
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/../Release/Tutorial/${CATEGORY}/UIRes)
        execute_process(COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/../Release/Tutorial/${CATEGORY}/UIRes
            ${CMAKE_CURRENT_BINARY_DIR}/${CATEGORY}/UIRes)
    endif()
endforeach()
