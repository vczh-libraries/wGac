#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"

usage() {
    echo "Usage: ./build.sh [--rebuild]" >&2
}

case "$#" in
    0)
        ;;
    1)
        if [[ "$1" != "--rebuild" ]]; then
            usage
            exit 1
        fi
        rm -rf -- "$BUILD_DIR"
        ;;
    *)
        usage
        exit 1
        ;;
esac

CC="${CC:-clang}" CXX="${CXX:-clang++}" cmake \
    -S "$SCRIPT_DIR" \
    -B "$BUILD_DIR"
cmake --build "$BUILD_DIR"
