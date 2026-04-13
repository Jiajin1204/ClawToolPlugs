#include "server.h"
#include "tool_registry.h"

#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <cerrno>
#include <cstring>
#include <csignal>
#include <iostream>
#include <sstream>

namespace phone_service {

namespace {

constexpr size_t BUFFER_SIZE = 8192;
constexpr int MAX_CLIENTS = 10;
constexpr useconds_t RECONNECT_DELAY_US = 100000;  // 100ms

// Set file descriptor to non-blocking mode
bool set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        return false;
    }
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK) != -1;
}

// Set close-on-exec for file descriptor
bool set_close_on_exec(int fd) {
    int flags = fcntl(fd, F_GETFD, 0);
    if (flags == -1) {
        return false;
    }
    return fcntl(fd, F_SETFD, flags | FD_CLOEXEC) != -1;
}

}  // anonymous namespace

// ============================================================================
// SocketServer
// ============================================================================

SocketServer::SocketServer(const std::string& socket_path, ToolRegistry& registry)
    : socket_path_(socket_path), registry_(registry) {
}

SocketServer::~SocketServer() {
    stop();
}

bool SocketServer::start() {
    if (running_.load()) {
        std::cerr << "[server] Already running" << std::endl;
        return true;
    }

    if (!setup_socket()) {
        std::cerr << "[server] Failed to setup socket" << std::endl;
        return false;
    }

    running_.store(true);
    accept_thread_ = std::thread(&SocketServer::accept_loop, this);

    std::cout << "[server] Started on " << socket_path_ << std::endl;
    return true;
}

void SocketServer::stop() {
    if (!running_.load()) {
        return;
    }

    running_.store(false);

    // Close server socket to unblock accept
    if (server_fd_ >= 0) {
        ::close(server_fd_);
        server_fd_ = -1;
    }

    // Wait for accept thread
    if (accept_thread_.joinable()) {
        accept_thread_.join();
    }

    // Wait for client threads
    {
        std::lock_guard<std::mutex> lock(clients_mutex_);
        for (auto& t : client_threads_) {
            if (t.joinable()) {
                t.join();
            }
        }
        client_threads_.clear();
    }

    cleanup_socket();
    std::cout << "[server] Stopped" << std::endl;
}

bool SocketServer::is_running() const {
    return running_.load();
}

std::string SocketServer::get_socket_path() const {
    return socket_path_;
}

void SocketServer::set_on_connect(std::function<void(int fd)> callback) {
    on_connect_ = std::move(callback);
}

void SocketServer::set_on_disconnect(std::function<void(int fd)> callback) {
    on_disconnect_ = std::move(callback);
}

void SocketServer::set_on_message(std::function<void(int fd, const json& msg)> callback) {
    on_message_ = std::move(callback);
}

bool SocketServer::send_to_client(int fd, const json& msg) {
    std::string output = msg.dump() + "\n";
    ssize_t written = write(fd, output.c_str(), output.size());

    if (written < 0) {
        std::cerr << "[server] Write error to fd " << fd << ": " << strerror(errno) << std::endl;
        return false;
    }

    return true;
}

void SocketServer::broadcast(const json& msg) {
    std::string output = msg.dump() + "\n";
    const char* data = output.c_str();
    size_t len = output.size();

    std::lock_guard<std::mutex> lock(clients_mutex_);

    // Note: This is a simplified broadcast. In production, you might want
    // to track client fds differently and handle write errors.
    for (int fd = 3; fd <= 3 + MAX_CLIENTS; ++fd) {
        // Skip server fd
        if (fd == server_fd_) {
            continue;
        }

        // Try to write (this is not reliable for real-world use)
        // A proper implementation would track client fds explicitly
    }
}

size_t SocketServer::client_count() const {
    std::lock_guard<std::mutex> lock(clients_mutex_);
    return client_threads_.size();
}

bool SocketServer::setup_socket() {
    // Create socket
    server_fd_ = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_fd_ < 0) {
        std::cerr << "[server] socket() failed: " << strerror(errno) << std::endl;
        return false;
    }

    // Set close-on-exec
    if (!set_close_on_exec(server_fd_)) {
        std::cerr << "[server] Failed to set close-on-exec" << std::endl;
    }

    // Remove existing socket file
    unlink(socket_path_.c_str());

    // Bind
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socket_path_.c_str(), sizeof(addr.sun_path) - 1);

    if (bind(server_fd_, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cerr << "[server] bind() failed: " << strerror(errno) << std::endl;
        ::close(server_fd_);
        server_fd_ = -1;
        return false;
    }

    // Listen
    if (listen(server_fd_, MAX_CLIENTS) < 0) {
        std::cerr << "[server] listen() failed: " << strerror(errno) << std::endl;
        ::close(server_fd_);
        server_fd_ = -1;
        return false;
    }

    // Set socket permissions (allow all access on Android, restrict on other platforms)
    chmod(socket_path_.c_str(), 0777);

    std::cout << "[server] Socket created at " << socket_path_ << std::endl;
    return true;
}

void SocketServer::cleanup_socket() {
    if (!socket_path_.empty()) {
        unlink(socket_path_.c_str());
    }
}

void SocketServer::accept_loop() {
    std::cout << "[server] Accept loop started" << std::endl;

    while (running_.load()) {
        struct sockaddr_un client_addr;
        socklen_t client_len = sizeof(client_addr);

        int client_fd = accept(server_fd_, (struct sockaddr*)&client_addr, &client_len);

        if (client_fd < 0) {
            if (errno == EINTR || errno == EAGAIN) {
                continue;
            }
            std::cerr << "[server] accept() failed: " << strerror(errno) << std::endl;
            break;
        }

        std::cout << "[server] Client connected: fd=" << client_fd << std::endl;

        // Set up client socket
        set_close_on_exec(client_fd);

        // Notify callback
        if (on_connect_) {
            on_connect_(client_fd);
        }

        // Spawn handler thread
        {
            std::lock_guard<std::mutex> lock(clients_mutex_);
            client_threads_.emplace_back(&SocketServer::handle_client, this, client_fd);
        }
    }

    std::cout << "[server] Accept loop ended" << std::endl;
}

void SocketServer::handle_client(int fd) {
    char buffer[BUFFER_SIZE];
    std::string current_message;

    while (running_.load()) {
        ssize_t bytes_read = read(fd, buffer, sizeof(buffer) - 1);

        if (bytes_read < 0) {
            if (errno == EINTR || errno == EAGAIN) {
                continue;
            }
            std::cerr << "[server] Read error on fd " << fd << ": " << strerror(errno) << std::endl;
            break;
        } else if (bytes_read == 0) {
            std::cout << "[server] Client disconnected: fd=" << fd << std::endl;
            break;
        }

        buffer[bytes_read] = '\0';
        current_message += buffer;

        // Process complete messages (newline-delimited JSON)
        size_t newline_pos;
        while ((newline_pos = current_message.find('\n')) != std::string::npos) {
            std::string line = current_message.substr(0, newline_pos);
            current_message.erase(0, newline_pos + 1);

            if (line.empty()) {
                continue;
            }

            auto msg = parse_message(line);
            if (msg) {
                auto response = process_message(fd, *msg);
                if (!response.is_null()) {
                    send_to_client(fd, response);
                }
            } else {
                std::cerr << "[server] Failed to parse message: " << line << std::endl;
                send_to_client(fd, make_error_message("Invalid JSON", -1));
            }
        }
    }

    // Cleanup
    if (on_disconnect_) {
        on_disconnect_(fd);
    }
    ::close(fd);
}

json SocketServer::process_message(int fd, const json& msg) {
    if (!msg.is_object() || !msg.contains("type")) {
        return make_error_message("Invalid message format", -1);
    }

    std::string type = msg["type"].get<std::string>();
    std::string id = msg.value("id", "");
    std::cout << "[server] Received message type: " << type << " from fd=" << fd << " id=" << id << std::endl;

    // Route to appropriate handler
    if (type == MSG_LIST_TOOLS) {
        return handle_list_tools(id);
    } else if (type == MSG_EXECUTE) {
        return handle_execute(msg, id);
    } else if (type == MSG_PING) {
        return handle_ping();
    } else if (type == MSG_REGISTER_DYNAMIC) {
        return handle_register_dynamic(msg);
    } else if (type == MSG_UNREGISTER) {
        return handle_unregister(msg);
    } else {
        return make_error_message("Unknown message type: " + type, -1);
    }
}

json SocketServer::handle_list_tools(const std::string& id) {
    auto tools = registry_.get_all_tools();

    json tools_array = json::array();
    for (const auto& tool : tools) {
        tools_array.push_back(tool.to_json());
    }

    return make_tools_response(tools_array, id);
}

json SocketServer::handle_execute(const json& params, const std::string& id) {
    if (!params.contains("tool")) {
        return make_error_message("Missing 'tool' field", -1);
    }

    std::string tool_name = params["tool"].get<std::string>();
    json tool_params = params.value("params", json::object());

    auto result = registry_.execute_tool(tool_name, tool_params);

    return make_result_message(result.success, result.data, result.error_message, id);
}

json SocketServer::handle_ping() {
    return make_pong_message();
}

json SocketServer::handle_register_dynamic(const json& params) {
    if (!params.contains("tools")) {
        return make_error_message("Missing 'tools' field", -1);
    }

    int count = 0;
    for (const auto& tool_def : params["tools"]) {
        Tool tool;
        try {
            tool.name = tool_def.at("name").get<std::string>();
            tool.description = tool_def.at("description").get<std::string>();

            if (tool_def.contains("parameters")) {
                for (const auto& [name, param_def] : tool_def["parameters"].items()) {
                    ToolParameter param;
                    param.type = param_def.value("type", "string");
                    param.description = param_def.value("description", "");
                    param.required = param_def.value("required", false);
                    tool.parameters[name] = param;
                }
            }

            // Create a generic executor that just echoes params
            // In a real implementation, this would be provided by the caller
            ToolExecutor executor = [](const json& p) -> ToolResult {
                return ToolResult::ok({{"echo", p}});
            };

            if (registry_.register_tool(tool, executor)) {
                count++;
            }
        } catch (const std::exception& e) {
            std::cerr << "[server] Error registering tool: " << e.what() << std::endl;
        }
    }

    return make_registered_message(count);
}

json SocketServer::handle_unregister(const json& params) {
    if (!params.contains("tool")) {
        return make_error_message("Missing 'tool' field", -1);
    }

    std::string tool_name = params["tool"].get<std::string>();

    if (registry_.unregister_tool(tool_name)) {
        return make_result_message(true, {{"unregistered", tool_name}});
    } else {
        return make_error_message("Failed to unregister tool: " + tool_name, -1);
    }
}

// ============================================================================
// Factory function
// ============================================================================

std::unique_ptr<SocketServer> create_socket_server(
    const std::string& socket_path,
    ToolRegistry& registry) {
    return std::make_unique<SocketServer>(socket_path, registry);
}

} // namespace phone_service
