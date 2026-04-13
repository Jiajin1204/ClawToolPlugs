#!/bin/bash
# Build script for phone-service
# Usage: ./build.sh [linux|android|clean]

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
SRC_DIR="$SCRIPT_DIR/src"
BUILD_DIR="$SCRIPT_DIR/build"
NDK_DIR="/home/jason/android-ndk-r27d/android-ndk-r27d"
NLOHMANN_DIR="/home/jason/projects/ClawAgent/thirdparty/nlohmann"

# Source files
SOURCES="
$SRC_DIR/main.cpp
$SRC_DIR/server.cpp
$SRC_DIR/tool_registry.cpp
$SRC_DIR/protocol.cpp
$SRC_DIR/tools/search_images.cpp
$SRC_DIR/tools/get_device_info.cpp
$SRC_DIR/tools/send_notification.cpp
$SRC_DIR/tools/get_location.cpp
$SRC_DIR/tools/install_app.cpp
"

build_linux() {
    echo "=== Building for Linux ==="

    mkdir -p "$BUILD_DIR"
    cd "$BUILD_DIR"

    cmake "$SCRIPT_DIR" \
        -DCMAKE_BUILD_TYPE=Release \
        -DNLOHMANN_JSON_DIR="$NLOHMANN_DIR"

    make -j$(nproc)

    echo ""
    echo "=== Build complete ==="
    echo "Binary: $BUILD_DIR/phone_service"
    ls -la "$BUILD_DIR/phone_service"
    file "$BUILD_DIR/phone_service"
}

build_android() {
    echo "=== Building for Android (NDK) ==="

    TOOLCHAIN="$NDK_DIR/toolchains/llvm/prebuilt/linux-x86_64/bin"
    SYSROOT="$NDK_DIR/toolchains/llvm/prebuilt/linux-x86_64/sysroot"
    TARGET="aarch64-linux-android21"

    CC="$TOOLCHAIN/aarch64-linux-android21-clang++"
    CXX="$TOOLCHAIN/aarch64-linux-android21-clang++"

    mkdir -p "$BUILD_DIR"
    cd "$SCRIPT_DIR"

    echo "Compiling..."
    $CXX \
        -std=c++17 \
        --target=$TARGET \
        -fPIC \
        -D__ANDROID__ \
        --sysroot=$SYSROOT \
        -I"$SRC_DIR" \
        -I"$NLOHMANN_DIR/include" \
        -o "$BUILD_DIR/phone_service" \
        $SOURCES \
        --target=$TARGET \
        --sysroot=$SYSROOT \
        -L"$TOOLCHAIN/../aarch64-linux-android/lib" \
        -static-libstdc++ \
        -lc -lm -llog

    echo ""
    echo "=== Build complete ==="
    echo "Binary: $BUILD_DIR/phone_service"
    ls -la "$BUILD_DIR/phone_service"
    file "$BUILD_DIR/phone_service"
}

clean() {
    echo "=== Cleaning build ==="
    rm -rf "$BUILD_DIR"
    mkdir -p "$BUILD_DIR"
    echo "Clean complete"
}

case "${1:-}" in
    linux)
        build_linux
        ;;
    android)
        build_android
        ;;
    clean)
        clean
        ;;
    *)
        echo "Usage: $0 [linux|android|clean]"
        echo ""
        echo "  linux   - Build for Linux (x86_64)"
        echo "  android - Build for Android (aarch64) using NDK"
        echo "  clean   - Clean build directory"
        exit 1
        ;;
esac
