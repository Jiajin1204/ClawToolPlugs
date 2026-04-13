# OpenClaw Phone Tools Plugin

A plugin that connects OpenClaw to a phone native service via Unix socket, exposing device capabilities as tools.

## Overview

This plugin enables OpenClaw running on a mobile device (or connected to one) to access native phone capabilities through a Unix socket connection to a native C++ service.

```
┌─────────────────────────────────────────────────────────────────┐
│                         OpenClaw                                 │
│  ┌───────────────────────────────────────────────────────────┐  │
│  │                    phone-tools Plugin                       │  │
│  │  - Registers tools with OpenClaw                           │  │
│  │  - Communicates via Unix socket                            │  │
│  │  - Auto-reconnect on connection loss                       │  │
│  └───────────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────────┘
                              │ Unix Socket
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│                    Phone Native Service (C++)                     │
│  ┌───────────────────────────────────────────────────────────┐  │
│  │  - search_images: Search phone gallery                     │  │
│  │  - get_device_info: Battery, storage, network              │  │
│  │  - send_notification: Push notifications                   │  │
│  │  - get_location: GPS coordinates                           │  │
│  └───────────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────────┘
```

## Prerequisites

- OpenClaw gateway v2026.3.24-beta.2 or later
- Node.js 22+ (for building)
- Phone native service running on the device

## Installation

### For Users

```bash
openclaw plugins install @openclaw/phone-tools
```

### For Developers

```bash
cd phone-tools
npm install
npm run build
```

Then link or copy to your OpenClaw extensions directory.

## Configuration

Configure the plugin in your OpenClaw config file:

```json
{
  "plugins": {
    "phone-tools": {
      "socketPath": "/tmp/phone_service.sock",
      "reconnectDelayMs": 2000,
      "connectTimeoutMs": 5000,
      "requestTimeoutMs": 30000
    }
  }
}
```

### Configuration Options

| Option | Type | Default | Description |
|--------|------|---------|-------------|
| `socketPath` | string | `/tmp/phone_service.sock` | Path to Unix socket |
| `reconnectDelayMs` | number | `2000` | Reconnection delay in ms |
| `connectTimeoutMs` | number | `5000` | Connection timeout in ms |
| `requestTimeoutMs` | number | `30000` | Tool execution timeout in ms |

## Available Tools

All phone tools are prefixed with `phone_` when registered with OpenClaw.

### phone_search_images

Search for images in the phone gallery by keyword.

**Parameters:**
- `keyword` (string, required): Search keyword
- `limit` (integer, optional): Maximum results (default: 10)

**Example:**
```
Search my photos for sunset images
```

### phone_get_device_info

Get device information including battery, storage, memory, and network status.

**Parameters:** None

**Example:**
```
What's my phone's battery level?
```

### phone_send_notification

Send a notification to the phone.

**Parameters:**
- `title` (string, required): Notification title
- `body` (string, required): Notification body
- `urgency` (string, optional): "low", "normal", "high" (default: "normal")

**Example:**
```
Remind me to call mom in 30 minutes
```

### phone_get_location

Get current GPS location.

**Parameters:**
- `accuracy` (string, optional): "high", "balanced", "low" (default: "balanced")

**Example:**
```
Where am I right now?
```

## Architecture

### Socket Protocol

Communication is via newline-delimited JSON over Unix domain socket:

```json
// List tools
{"type": "list_tools"}

// Response
{"type": "tools", "tools": [...]}

// Execute tool
{"type": "execute", "tool": "search_images", "params": {"keyword": "sunset"}}

// Result
{"type": "result", "success": true, "data": {...}}
```

### Connection Management

- **Auto-reconnect**: Plugin automatically reconnects if connection is lost
- **Keep-alive**: Ping/pong messages every 30 seconds
- **Request timeout**: Configurable timeout for tool execution

## Development

### Project Structure

```
phone-tools/
├── package.json
├── tsconfig.json
├── openclaw.plugin.json     # Plugin manifest
├── api.ts                   # Public API exports
├── runtime-api.ts           # Runtime API exports
├── README.md                # This file
└── src/
    ├── index.ts             # Plugin entry point
    ├── types.ts             # TypeScript types
    ├── socket-client.ts      # Socket communication
    └── tool-registry.ts      # Tool registration
```

### Building

```bash
npm install
npm run build
```

### Testing

```bash
# Start the phone service (from phone-service directory)
cd ../phone-service
mkdir -p build && cd build
cmake .. && make
./phone_service &

# Test socket connection
echo '{"type":"list_tools"}' | nc -U /tmp/phone_service.sock

# Run plugin tests (when available)
npm test
```

## Troubleshooting

### Connection Issues

1. **Socket not found**: Ensure phone service is running
   ```bash
   ls -la /tmp/phone_service.sock
   ```

2. **Permission denied**: Check socket permissions
   ```bash
   chmod 777 /tmp/phone_service.sock
   ```

3. **Connection timeout**: Increase `connectTimeoutMs` in config

### Tool Execution Issues

1. **Tool not found**: Verify tool is registered
   ```
   Check OpenClaw logs for registered tools
   ```

2. **Timeout**: Increase `requestTimeoutMs` for slow operations

3. **Invalid params**: Check tool schema in OpenClaw logs

## Protocol Reference

See [phone-service/docs/protocol.md](../phone-service/docs/protocol.md) for detailed protocol specification.

## License

MIT
