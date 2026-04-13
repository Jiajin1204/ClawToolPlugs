#!/bin/bash
# Cross-compile phone-service for Android using NDK

NDK_PATH="/home/jason/android-ndk-r27d/android-ndk-r27d"
TOOLCHAIN="$NDK_PATH/toolchains/llvm/prebuilt/linux-x86_64/bin"
SYSROOT="$NDK_PATH/toolchains/llvm/prebuilt/linux-x86_64/sysroot"
LIB_DIR="$TOOLCHAIN/../aarch64-linux-android/lib"

# Android target
TARGET="aarch64-linux-android21"
CC="$TOOLCHAIN/aarch64-linux-android21-clang++"
CXX="$TOOLCHAIN/aarch64-linux-android21-clang++"

# Source directory
SRC_DIR="$(cd "$(dirname "$0")" && pwd)/src"
OUT_DIR="$(cd "$(dirname "$0")" && pwd)/android-build"
NLOHMANN_DIR="/home/jason/projects/ClawAgent/thirdparty/nlohmann/include"
mkdir -p "$OUT_DIR"

echo "Cross-compiling phone-service for Android..."
echo "Target: $TARGET"
echo "Source: $SRC_DIR"
echo "Output: $OUT_DIR"
echo "Sysroot: $SYSROOT"

# Common flags
CXXFLAGS="-std=c++17 --target=$TARGET -fPIC -D__ANDROID__ --sysroot=$SYSROOT"
# Link with static C++ runtime to avoid needing shared libs
LDFLAGS="--target=$TARGET --sysroot=$SYSROOT -L$LIB_DIR -static-libstdc++ -lc -lm -llog"

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

# Include paths
INCLUDES="-I$SRC_DIR -I$NLOHMANN_DIR"

echo "Compiling..."
$CC $CXXFLAGS $INCLUDES -o "$OUT_DIR/phone_service" $SOURCES $LDFLAGS

if [ $? -eq 0 ]; then
    echo "Success! Output: $OUT_DIR/phone_service"
    ls -la "$OUT_DIR/phone_service"
    file "$OUT_DIR/phone_service"
else
    echo "Compilation failed!"
    exit 1
fi
