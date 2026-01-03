cmake_minimum_required(VERSION 3.20)
project(wGac VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)

# Find required packages
find_package(PkgConfig REQUIRED)

pkg_check_modules(WAYLAND REQUIRED wayland-client wayland-cursor)
pkg_check_modules(CAIRO REQUIRED cairo)
pkg_check_modules(PANGO REQUIRED pango pangocairo)
pkg_check_modules(XKB REQUIRED xkbcommon)

# Wayland protocols
find_program(WAYLAND_SCANNER wayland-scanner REQUIRED)

set(WAYLAND_PROTOCOLS_DIR "/usr/share/wayland-protocols")

# Generate protocol files
set(PROTOCOL_GENERATED_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Protocol/generated")

# xdg-shell protocol
set(XDG_SHELL_XML "${WAYLAND_PROTOCOLS_DIR}/stable/xdg-shell/xdg-shell.xml")
set(XDG_SHELL_CLIENT_HEADER "${PROTOCOL_GENERATED_DIR}/xdg-shell-client-protocol.h")
set(XDG_SHELL_CLIENT_CODE "${PROTOCOL_GENERATED_DIR}/xdg-shell-protocol.c")

add_custom_command(
    OUTPUT ${XDG_SHELL_CLIENT_HEADER}
    COMMAND ${WAYLAND_SCANNER} client-header ${XDG_SHELL_XML} ${XDG_SHELL_CLIENT_HEADER}
    DEPENDS ${XDG_SHELL_XML}
    COMMENT "Generating xdg-shell client header"
)

add_custom_command(
    OUTPUT ${XDG_SHELL_CLIENT_CODE}
    COMMAND ${WAYLAND_SCANNER} private-code ${XDG_SHELL_XML} ${XDG_SHELL_CLIENT_CODE}
    DEPENDS ${XDG_SHELL_XML}
    COMMENT "Generating xdg-shell client code"
)

# xdg-decoration protocol (for server-side decorations)
set(XDG_DECORATION_XML "${WAYLAND_PROTOCOLS_DIR}/unstable/xdg-decoration/xdg-decoration-unstable-v1.xml")
set(XDG_DECORATION_CLIENT_HEADER "${PROTOCOL_GENERATED_DIR}/xdg-decoration-unstable-v1-client-protocol.h")
set(XDG_DECORATION_CLIENT_CODE "${PROTOCOL_GENERATED_DIR}/xdg-decoration-unstable-v1-protocol.c")

add_custom_command(
    OUTPUT ${XDG_DECORATION_CLIENT_HEADER}
    COMMAND ${WAYLAND_SCANNER} client-header ${XDG_DECORATION_XML} ${XDG_DECORATION_CLIENT_HEADER}
    DEPENDS ${XDG_DECORATION_XML}
    COMMENT "Generating xdg-decoration client header"
)

add_custom_command(
    OUTPUT ${XDG_DECORATION_CLIENT_CODE}
    COMMAND ${WAYLAND_SCANNER} private-code ${XDG_DECORATION_XML} ${XDG_DECORATION_CLIENT_CODE}
    DEPENDS ${XDG_DECORATION_XML}
    COMMENT "Generating xdg-decoration client code"
)

# Protocol sources
set(PROTOCOL_SOURCES
    ${XDG_SHELL_CLIENT_HEADER}
    ${XDG_SHELL_CLIENT_CODE}
    ${XDG_DECORATION_CLIENT_HEADER}
    ${XDG_DECORATION_CLIENT_CODE}
)

# Source files
set(WAYLAND_SOURCES
    Source/Wayland/WaylandDisplay.cpp
    Source/Wayland/WaylandBuffer.cpp
    Source/Wayland/WaylandSeat.cpp
)

set(CORE_SOURCES
    Source/WGacWindow.cpp
    Source/WGacView.cpp
    Source/WGacCursor.cpp
)

set(RENDERER_SOURCES
    Source/Renderers/WGacRenderer.cpp
)

# Main library
add_library(wGac_lib STATIC
    ${PROTOCOL_SOURCES}
    ${WAYLAND_SOURCES}
    ${CORE_SOURCES}
    ${RENDERER_SOURCES}
)

target_include_directories(wGac_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/Source
    ${CMAKE_CURRENT_SOURCE_DIR}/Protocol/generated
    ${CMAKE_CURRENT_SOURCE_DIR}/Release/Import
    ${WAYLAND_INCLUDE_DIRS}
    ${CAIRO_INCLUDE_DIRS}
    ${PANGO_INCLUDE_DIRS}
    ${XKB_INCLUDE_DIRS}
)

target_link_libraries(wGac_lib PUBLIC
    ${WAYLAND_LIBRARIES}
    ${CAIRO_LIBRARIES}
    ${PANGO_LIBRARIES}
    ${XKB_LIBRARIES}
    rt  # for shm_open
)

# Compile options
target_compile_options(wGac_lib PRIVATE -Wall -Wextra)
