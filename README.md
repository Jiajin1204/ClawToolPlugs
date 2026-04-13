# ClawToolPlugs

本项目包含两个组件，用于将手机原生能力暴露给 OpenClaw AI Agent：

- **phone-service**：C++ 原生服务，通过 Unix Socket 提供手机工具能力
- **phone-tools**：TypeScript 插件，连接 phone-service 并将工具注册到 OpenClaw

## 架构图

```
┌─────────────────────────────────────────────────────────────┐
│                        OpenClaw                              │
│  ┌─────────────────────────────────────────────────────┐    │
│  │                   phone-tools 插件                    │    │
│  │  - 连接 phone-service (Unix Socket)                  │    │
│  │  - 注册工具到 OpenClaw (phone_xxx)                   │    │
│  │  - 自动重连机制                                      │    │
│  └─────────────────────────────────────────────────────┘    │
└─────────────────────────────────────────────────────────────┘
                              │ Unix Socket
                              ▼
┌─────────────────────────────────────────────────────────────┐
│                    phone-service (C++)                       │
│  ┌─────────────────────────────────────────────────────┐    │
│  │  search_images    - 搜索相册图片                       │    │
│  │  get_device_info  - 获取设备信息                       │    │
│  │  send_notification - 发送通知                         │    │
│  │  get_location     - 获取 GPS 位置                      │    │
│  │  install_app       - 安装 APK                         │    │
│  └─────────────────────────────────────────────────────┘    │
└─────────────────────────────────────────────────────────────┘
```

## 项目结构

```
ClawToolPlugs/
├── phone-service/           # C++ 原生服务
│   ├── src/                 # 源代码
│   │   ├── main.cpp         # 入口
│   │   ├── server.cpp/h     # Unix Socket 服务器
│   │   ├── protocol.cpp/h   # JSON 协议处理
│   │   ├── tool_registry.cpp/h  # 工具注册
│   │   └── tools/           # 各个工具实现
│   ├── build.sh             # 编译脚本
│   ├── tools.json           # 静态工具配置
│   └── CMakeLists.txt
├── phone-tools/             # TypeScript 插件
│   ├── src/
│   │   ├── index.ts         # 插件入口
│   │   ├── socket-client.ts # Socket 客户端
│   │   └── types.ts         # 类型定义
│   ├── package.json
│   └── tsconfig.json
└── README.md
```

---

## 一、编译 phone-service

### Linux 本地编译

```bash
cd phone-service
./build.sh linux
```

编译产物：`phone-service/build/phone_service`

### Android 交叉编译（需要 Android NDK）

```bash
cd phone-service
./build.sh android
```

编译产物：`phone-service/build/phone_service`（ARM aarch64）

### 单独使用 NDK 编译

```bash
cd phone-service
rm -rf build
mkdir build && cd build

cmake -DCMAKE_TOOLCHAIN_FILE=${ANDROID_NDK_HOME}/build/cmake/android-ndk.cmake \
      -DANDROID_ABI=arm64-v8a \
      -DANDROID_PLATFORM=android-21 \
      ..

make -j$(nproc)
```

---

## 二、编译 phone-tools

```bash
cd phone-tools
npm install
npm run build
```

编译产物：`phone-tools/dist/`

---

## 三、安装到新手机

假设手机 IP 为 `192.168.1.100`，使用 ADB 或其他方式传输文件。

### 3.1 部署 phone-service

```bash
# 创建目录
adb shell mkdir -p /data/data/com.termux/files/home/phone-service

# 上传可执行文件和配置
adb push build/phone_service /data/data/com.termux/files/home/phone-service/
adb push tools.json /data/data/com.termux/files/home/phone-service/

# 设置执行权限
adb shell chmod +x /data/data/com.termux/files/home/phone-service/phone_service
```

### 3.2 部署 phone-tools

```bash
# 上传插件
adb push dist/ /data/data/com.termux/files/home/openclaw-plugins/phone-tools/
```

### 3.3 启动 phone-service

```bash
# 在手机 Termux 中执行
cd /data/data/com.termux/files/home/phone-service
./phone_service -s phone_service.sock -c tools.json &

# 确认启动成功
logcat | grep phone_service
# 应看到: [server] Started on phone_service.sock
```

---

## 四、配置 OpenClaw

编辑 OpenClaw 配置文件 `openclaw.json`：

```bash
adb shell nano /data/data/com.termux/files/home/openclaw.json
```

添加插件配置：

```json
{
  "plugins": {
    "phone-tools": {
      "enabled": true,
      "config": {
        "socketPath": "/data/data/com.termux/files/home/phone-service/phone_service.sock"
      }
    }
  }
}
```

---

## 五、验证测试

### 5.1 检查 phone-service 运行状态

```bash
adb shell logcat | grep phone_service
```

应看到：
```
[server] Started on phone_service.sock
```

### 5.2 重启 OpenClaw

```bash
adb shell pkill openclaw
adb shell openclaw gateway &
```

### 5.3 检查插件加载

```bash
adb shell logcat | grep phone-tools
```

应看到：
```
[phone-tools] Registering phone-tools plugin
[phone-tools] Connected to phone service
[phone-tools] Received 5 tools from phone service
[phone-tools] Registered tool: phone_search_images
[phone-tools] Registered tool: phone_get_device_info
[phone-tools] Registered tool: phone_send_notification
[phone-tools] Registered tool: phone_get_location
[phone-tools] Registered tool: phone_install_app
```

### 5.4 测试工具执行

在 OpenClaw 对话中测试：

```
用户: 搜索我相册里猫咪的照片
助手: (调用 phone_search_images)

用户: 我的手机电量是多少？
助手: (调用 phone_get_device_info)

用户: 给我发个通知测试
助手: (调用 phone_send_notification)
```

---

## 六、可用工具

| 工具名 | 说明 | 参数 |
|--------|------|------|
| `phone_search_images` | 搜索相册图片 | `keyword`（必填），`limit`（可选，默认10） |
| `phone_get_device_info` | 获取设备信息 | 无 |
| `phone_send_notification` | 发送通知 | `title`（必填），`body`（必填），`urgency`（可选） |
| `phone_get_location` | 获取 GPS 位置 | `accuracy`（可选：high/balanced/low） |
| `phone_install_app` | 安装 APK | `apk_path`（必填），`method`（可选：auto/pm/cmd） |

---

## 七、常见问题

| 问题 | 解决方法 |
|------|----------|
| `phone_service: command not found` | 执行 `chmod +x phone_service` |
| `Connection refused` | 检查 socket 文件是否存在 |
| `Timeout 30s` | 检查 phone-service 是否正常响应请求 |
| `Permission denied` | 执行 `chmod 777 phone_service.sock` |

### 查看日志

```bash
# phone-service 日志
adb shell logcat | grep phone_service

# OpenClaw 日志
adb shell logcat | grep openclaw

# 实时查看所有相关日志
adb shell logcat | grep -E "phone_service|phone-tools|openclaw"
```

### 停止服务

```bash
# 停止 phone-service
adb shell pkill phone_service

# 停止 OpenClaw
adb shell pkill openclaw
```

---

## 八、开发说明

### 协议格式

通信使用 Unix Socket，JSON 格式（每条消息以换行符分隔）：

```json
// 请求工具列表
{"type": "list_tools", "id": "req_1"}

// 响应工具列表
{"type": "tools", "tools": [...], "id": "req_1"}

// 执行工具
{"type": "execute", "tool": "xxx", "params": {...}, "id": "req_2"}

// 执行结果
{"type": "result", "success": true, "data": {...}, "id": "req_2"}
```

### 添加新工具

1. 在 `phone-service/src/tools/` 下创建新工具文件
2. 在 `tools.json` 中添加静态工具配置
3. 重新编译 phone-service
4. 重启服务

详细开发文档待补充。
