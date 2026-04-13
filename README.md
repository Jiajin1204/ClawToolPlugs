# Phone Native Service

A C++ Unix socket server that provides tool capabilities to OpenClaw.

## Overview

This service runs on a mobile device and exposes native device capabilities (camera, GPS, storage, etc.) as tools via Unix socket. The OpenClaw `phone-tools` plugin connects to this service to access these tools.

## Prerequisites

- C++17 compatible compiler (GCC 9+, Clang 10+, or MSVC 2019+)
- CMake 3.16+
- nlohmann/json v3.11.3+

### Installing Dependencies

**Ubuntu/Debian:**
```bash
sudo apt install cmake build-essential nlohmann-json3-dev
```

**macOS:**
```bash
brew install cmake nlohmann-json
```

**Android (with NDK):**
```bash
# Set up Android NDK environment
export ANDROID_NDK_HOME=/path/to/ndk
```

**Using vcpkg:**
```bash
vcpkg install nlohmann-json:x64-linux
```

## Project Structure

```
phone-service/
├── CMakeLists.txt          # Build configuration
├── tools.json              # Static tool configuration
├── README.md               # This file
├── docs/
│   └── protocol.md         # Communication protocol docs
└── src/
    ├── main.cpp            # Application entry point
    ├── server.h/cpp        # Unix socket server
    ├── tool_registry.h/cpp # Tool registration and execution
    ├── protocol.h/cpp      # JSON protocol handling
    └── tools/
        ├── json.hpp        # nlohmann/json header
        ├── search_images.h/cpp   # Image search tool
        ├── get_device_info.h/cpp # Device info tool
        ├── send_notification.h/cpp # Notification tool
        └── get_location.h/cpp     # Location tool
```

## Building

### Standard Build

```bash
# Create build directory
mkdir build && cd build

# Configure
cmake ..

# Build
make -j$(nproc)

# Install (optional)
sudo make install
```

### Android NDK Build

```bash
# Create standalone toolchain (one-time setup)
${ANDROID_NDK_HOME}/build/tools/make-standalone-toolchain.sh \
    --arch arm64 \
    --install-dir=/opt/android-toolchain

# Configure with toolchain
mkdir build && cd build
cmake -DCMAKE_TOOLCHAIN_FILE=/opt/android-toolchain/sysroot/usr/share/cmake/android-ndk.cmake \
      -DANDROID_ABI=arm64-v8a \
      -DANDROID_PLATFORM=android-24 \
      ..

make
```

## Running

```bash
# Default socket path
./phone_service

# Custom socket path
./phone_service --socket /data/local/tmp/phone_service.sock

# Custom tools config
./phone_service --config /path/to/tools.json

# Full options
./phone_service -s /tmp/phone_service.sock -c tools.json
```

## Available Tools

### search_images
Search for images by keyword in the phone gallery.

**Parameters:**
- `keyword` (string, required): Search keyword
- `limit` (integer, optional): Max results (default: 10)

**Example:**
```json
{
  "type": "execute",
  "tool": "search_images",
  "params": {
    "keyword": "sunset",
    "limit": 5
  }
}
```

### get_device_info
Get device information (battery, storage, network).

**Parameters:** None

**Example:**
```json
{
  "type": "execute",
  "tool": "get_device_info",
  "params": {}
}
```

### send_notification
Send a notification to the phone.

**Parameters:**
- `title` (string, required): Notification title
- `body` (string, required): Notification body
- `urgency` (string, optional): "low", "normal", "high" (default: "normal")

### get_location
Get current GPS location.

**Parameters:**
- `accuracy` (string, optional): "high", "balanced", "low" (default: "balanced")

## Protocol

Communication is via newline-delimited JSON over Unix domain socket.

### Message Types

| Type | Direction | Description |
|------|----------|-------------|
| `list_tools` | → | Request tool list |
| `tools` | ← | Tool list response |
| `execute` | → | Execute a tool |
| `result` | ← | Execution result |
| `error` | ← | Error response |
| `ping` | → | Keep-alive ping |
| `pong` | ← | Keep-alive response |
| `register_dynamic` | → | Dynamic tool registration |
| `unregister` | → | Unregister a tool |

See [docs/protocol.md](docs/protocol.md) for full protocol specification.

## Static vs Dynamic Tools

**Static tools** are defined in `tools.json` and loaded at startup. These are compiled-in capabilities.

**Dynamic tools** can be registered at runtime via the `register_dynamic` message. These are temporary and not persisted.

## Signal Handling

The service handles SIGINT and SIGTERM for clean shutdown:

```bash
# Send termination signal
kill -TERM $(pidof phone_service)
```

## Testing

```bash
# Test with netcat
nc -U /tmp/phone_service.sock

# Send list_tools request
echo '{"type":"list_tools"}' | nc -U /tmp/phone_service.sock

# Send execute request
echo '{"type":"execute","tool":"get_device_info","params":{}}' | nc -U /tmp/phone_service.sock
```

## License

MIT
