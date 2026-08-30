#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"

REQUIRED_PKG_CONFIG_MODULES=(
    wayland-client
    wayland-cursor
    xkbcommon
    cairo
    pangocairo
    fontconfig
    gio-2.0
    liburing
    libdecor-0
)

DEBIAN_BUILD_PACKAGES=(
    pkg-config
    pkgconf
    pkgconf-bin
    libpkgconf3
    libffi-dev
    libblkid-dev
    libbrotli-dev
    libbz2-dev
    libdatrie-dev
    libexpat1-dev
    libfontconfig-dev
    libfreetype-dev
    libfribidi-dev
    libgraphite2-dev
    libharfbuzz-dev
    libice-dev
    libmount-dev
    libpcre2-dev
    libpixman-1-dev
    libpng-dev
    libpthread-stubs0-dev
    libselinux1-dev
    libsepol-dev
    libsm-dev
    libthai-dev
    libwayland-dev
    libxkbcommon-dev
    libdecor-0-dev
    libcairo2-dev
    libpango1.0-dev
    libglib2.0-dev
    liburing-dev
    libx11-dev
    libxau-dev
    libxcb-render0-dev
    libxcb-shm0-dev
    libxcb1-dev
    libxdmcp-dev
    libxext-dev
    libxft-dev
    libxrender-dev
    uuid-dev
    x11proto-dev
    xtrans-dev
    zlib1g-dev
)

usage() {
    echo "Usage: ./build.sh [--rebuild]" >&2
}

pkg_config_has_required_modules() {
    local pkg_config_executable="$1"
    "$pkg_config_executable" --exists "${REQUIRED_PKG_CONFIG_MODULES[@]}"
}

configure_bootstrapped_pkg_config() {
    local dependencies_root="$1"
    local pkg_config_directories="$dependencies_root/usr/share/pkgconfig"
    local library_directories="$dependencies_root/usr/lib:$dependencies_root/lib"
    local directory

    while IFS= read -r directory; do
        pkg_config_directories="$directory:$pkg_config_directories"
    done < <(find "$dependencies_root/usr/lib" -type d -name pkgconfig -print)

    while IFS= read -r directory; do
        library_directories="$directory:$library_directories"
    done < <(find "$dependencies_root/usr/lib" "$dependencies_root/lib" -mindepth 1 -maxdepth 1 -type d -print 2>/dev/null)

    export PKG_CONFIG_SYSROOT_DIR="$dependencies_root"
    export PKG_CONFIG_LIBDIR="$pkg_config_directories"
    export LD_LIBRARY_PATH="$library_directories${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
    PKG_CONFIG_EXECUTABLE="$dependencies_root/usr/bin/pkg-config"
}

repair_bootstrapped_library_links() {
    local dependencies_root="$1"
    local dependency_link
    local dependency_name
    local dependency_path
    local ldconfig_cache

    ldconfig_cache="$(ldconfig -p)"
    while IFS= read -r dependency_link; do
        dependency_name="$(basename "$dependency_link")"
        dependency_path="$(awk -v prefix="$dependency_name." 'index($1, prefix) == 1 { print $NF; exit }' <<< "$ldconfig_cache")"
        if [[ -n "$dependency_path" ]]; then
            ln -sfn "$dependency_path" "$dependency_link"
        fi
    done < <(find "$dependencies_root/usr/lib" -xtype l -name '*.so' -print)
}

bootstrap_debian_dependencies() {
    local dependencies_directory="$BUILD_DIR/dependencies"
    local dependencies_root="$dependencies_directory/root"
    local apt_archives="$dependencies_directory/apt/archives"
    local apt_sandbox_user
    local dependency_package
    local required_command

    for required_command in apt-get dpkg-deb ldconfig; do
        if ! command -v "$required_command" >/dev/null 2>&1; then
            echo "Missing $required_command. Install the packages listed in README.md and run ./build.sh again." >&2
            exit 1
        fi
    done

    mkdir -p \
        "$apt_archives/partial" \
        "$dependencies_root/lib" \
        "$dependencies_root/usr/lib" \
        "$dependencies_root/usr/share/pkgconfig"
    configure_bootstrapped_pkg_config "$dependencies_root"
    repair_bootstrapped_library_links "$dependencies_root"
    if [[ -x "$PKG_CONFIG_EXECUTABLE" ]] && pkg_config_has_required_modules "$PKG_CONFIG_EXECUTABLE"; then
        return
    fi

    echo "System development packages are unavailable; downloading a build-local Debian/Ubuntu dependency set." >&2
    apt_sandbox_user="$(id -un)"
    if ! apt-get \
        -y \
        -o Debug::NoLocking=1 \
        -o "APT::Sandbox::User=$apt_sandbox_user" \
        -o "Dir::Cache::archives=$apt_archives" \
        --download-only \
        --no-install-recommends \
        --reinstall \
        install "${DEBIAN_BUILD_PACKAGES[@]}"; then
        echo "Unable to download build dependencies. Refresh apt metadata or install the packages listed in README.md." >&2
        exit 1
    fi

    for dependency_package in "$apt_archives"/*.deb; do
        dpkg-deb -x "$dependency_package" "$dependencies_root"
    done

    repair_bootstrapped_library_links "$dependencies_root"

    configure_bootstrapped_pkg_config "$dependencies_root"
    if [[ ! -x "$PKG_CONFIG_EXECUTABLE" ]] || ! pkg_config_has_required_modules "$PKG_CONFIG_EXECUTABLE"; then
        echo "The downloaded dependency set is incomplete. Install the packages listed in README.md." >&2
        exit 1
    fi
}

resolve_pkg_config() {
    local pkg_config_candidate=""

    if [[ -n "${PKG_CONFIG_EXECUTABLE:-}" && -x "$PKG_CONFIG_EXECUTABLE" ]]; then
        pkg_config_candidate="$PKG_CONFIG_EXECUTABLE"
    elif command -v pkg-config >/dev/null 2>&1; then
        pkg_config_candidate="$(command -v pkg-config)"
    elif command -v pkgconf >/dev/null 2>&1; then
        pkg_config_candidate="$(command -v pkgconf)"
    fi

    if [[ -n "$pkg_config_candidate" ]] && pkg_config_has_required_modules "$pkg_config_candidate"; then
        PKG_CONFIG_EXECUTABLE="$pkg_config_candidate"
        return
    fi

    if [[ ! -f /etc/debian_version ]]; then
        echo "Required pkg-config modules are unavailable. Install the packages listed in README.md." >&2
        exit 1
    fi

    bootstrap_debian_dependencies
}

cd "$SCRIPT_DIR"

case "$#" in
    0)
        ;;
    1)
        if [[ "$1" != "--rebuild" ]]; then
            usage
            exit 1
        fi
        git clean -xdf
        ;;
    *)
        usage
        exit 1
        ;;
esac

mkdir -p "$BUILD_DIR"
resolve_pkg_config

CC="${CC:-clang}" CXX="${CXX:-clang++}" cmake \
    -S "$SCRIPT_DIR" \
    -B "$BUILD_DIR" \
    -D "PKG_CONFIG_EXECUTABLE:FILEPATH=$PKG_CONFIG_EXECUTABLE"
cmake --build "$BUILD_DIR"
