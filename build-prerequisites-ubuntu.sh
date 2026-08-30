#!/usr/bin/env bash

set -euo pipefail

if [[ "$#" -ne 0 ]]; then
    echo "Usage: ./build-prerequisites-ubuntu.sh" >&2
    exit 1
fi

if [[ ! -r /etc/os-release ]]; then
    echo "Unable to identify the operating system. This script supports Ubuntu only." >&2
    exit 1
fi

# shellcheck disable=SC1091
source /etc/os-release
if [[ "${ID:-}" != "ubuntu" ]]; then
    echo "This script supports Ubuntu only. Install the equivalent packages for your distribution." >&2
    exit 1
fi

PACKAGES=(
    build-essential
    clang
    cmake
    pkg-config
    libwayland-dev
    libxkbcommon-dev
    libdecor-0-dev
    libdecor-0-plugin-1-gtk
    libcairo2-dev
    libpango1.0-dev
    libfontconfig-dev
    libgdk-pixbuf-2.0-dev
    libglib2.0-dev
    liburing-dev
)

if [[ "$EUID" -ne 0 ]]; then
    echo "Root privileges are required. Run: sudo ./build-prerequisites-ubuntu.sh" >&2
    exit 1
fi

apt-get update
apt-get install -y "${PACKAGES[@]}"

echo "wGac build prerequisites are installed. Run ./build.sh to build the project."
