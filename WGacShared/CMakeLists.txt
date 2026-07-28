cmake_minimum_required(VERSION 3.20)
project(WGacShared LANGUAGES C CXX)

find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)

pkg_check_modules(WAYLAND REQUIRED IMPORTED_TARGET wayland-client wayland-cursor)
pkg_check_modules(XKB REQUIRED IMPORTED_TARGET xkbcommon)
pkg_check_modules(CAIRO REQUIRED IMPORTED_TARGET cairo pangocairo)
pkg_check_modules(FONTCONFIG REQUIRED IMPORTED_TARGET fontconfig)
pkg_check_modules(GIO REQUIRED IMPORTED_TARGET gio-2.0)
pkg_check_modules(URING REQUIRED IMPORTED_TARGET liburing)
pkg_check_modules(LIBDECOR REQUIRED IMPORTED_TARGET libdecor-0)

set(GACUI_COMMON_FILES
    "../Import/Vlpp.cpp"
    "../Import/Vlpp.Linux.cpp"
    "../Import/VlppOS.cpp"
    "../Import/VlppOS.Linux.cpp"
    "../Import/VlppRegex.cpp"
    "../Import/VlppReflection.cpp"
    "../Import/VlppGlrParser.cpp"
    "../Import/VlppWorkflowLibrary.cpp"
    "../Import/GacUI.cpp"
    "../Import/Skins/DarkSkin/DarkSkin.cpp"
)

set(WGAC_FILES
    "../WGac/Services/WGacCallbackService.cpp"
    "../WGac/Services/WGacInputService.cpp"
    "../WGac/Services/WGacAsyncService.cpp"
    "../WGac/Services/WGacImageService.cpp"
    "../WGac/Services/WGacDialogService.cpp"
    "../WGac/Services/WGacResourceService.cpp"
    "../WGac/Services/WGacScreenService.cpp"
    "../WGac/Services/WGacClipboardService.cpp"
    "../WGac/Services/WGacAutomationService.cpp"
    "../WGac/Renderers/WGacRendererImpl.cpp"
    "../WGac/Wayland/WaylandDisplay.cpp"
    "../WGac/Wayland/WaylandBuffer.cpp"
    "../WGac/Wayland/WaylandSeat.cpp"
    "../WGac/WGacController.cpp"
    "../WGac/WGacNativeWindow.cpp"
    "../WGac/WGacGacView.cpp"
    "../WGac/WGacWindow.cpp"
    "../WGac/WGacWindowView.cpp"
    "../WGac/WGacCursor.cpp"
    "../WGac/Protocol/xdg-shell-protocol.c"
    "../WGac/Protocol/xdg-decoration-protocol.c"
    "../WGac/Protocol/text-input-unstable-v3-protocol.c"
)

set(WGAC_SHARED_FILES
    "MiniHttpAutomationService.cpp"
)

add_library(GacUI STATIC ${GACUI_COMMON_FILES})
target_compile_definitions(GacUI PUBLIC VCZH_DEBUG_NO_REFLECTION)
target_include_directories(GacUI PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/../Import"
)
target_link_libraries(GacUI PUBLIC
    Threads::Threads
    PkgConfig::URING
)

add_library(WGac STATIC ${WGAC_FILES})
target_include_directories(WGac PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/../Import"
    "${CMAKE_CURRENT_SOURCE_DIR}/../WGac"
    "${CMAKE_CURRENT_SOURCE_DIR}/../WGac/Renderers"
    "${CMAKE_CURRENT_SOURCE_DIR}/../WGac/Services"
    "${CMAKE_CURRENT_SOURCE_DIR}/../WGac/Protocol"
)
target_link_libraries(WGac PUBLIC
    GacUI
    PkgConfig::WAYLAND
    PkgConfig::XKB
    PkgConfig::CAIRO
    PkgConfig::FONTCONFIG
    PkgConfig::GIO
    PkgConfig::LIBDECOR
    Threads::Threads
    ${CMAKE_DL_LIBS}
    rt
)

add_library(WGacShared STATIC ${WGAC_SHARED_FILES})
target_include_directories(WGacShared PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/../Import"
    "${CMAKE_CURRENT_SOURCE_DIR}/../WGac"
)
target_link_libraries(WGacShared PUBLIC GacUI)
