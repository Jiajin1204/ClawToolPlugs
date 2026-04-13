#pragma once

#include <string>
#include <memory>
#include <atomic>
#include <thread>
#include <mutex>
#include <functional>
#include <vector>

#include "protocol.h"
#include "tool_registry.h"

namespace phone_service {

class ToolRegistry;

/**
 * @brief Unix socket server for phone service
 *
 * Listens on a Unix domain socket and handles JSON protocol messages.
 * Supports:
 * - Multiple client connections
 * - Tool listing and execution
 * - Dynamic tool registration
 * - Connection keep-alive with ping/pong
 */
class SocketServer {
public:
    /**
     * @brief Construct a socket server
     * @param socket_path Path to Unix socket file
     * @param registry Tool registry reference
     */
    SocketServer(const std::string& socket_path, ToolRegistry& registry);
    ~SocketServer();

    // Non-copyable
    SocketServer(const SocketServer&) = delete;
    SocketServer& operator=(const SocketServer&) = delete;

    /**
     * @brief Start the server (blocking)
     * @return true if started successfully
     */
    bool start();

    /**
     * @brief Stop the server
     */
    void stop();

    /**
     * @brief Check if server is running
     * @return true if running
     */
    bool is_running() const;

    /**
     * @brief Get the socket path
     * @return Socket path
     */
    std::string get_socket_path() const;

    /**
     * @brief Set connection callback (called when client connects)
     * @param callback Function to call
     */
    void set_on_connect(std::function<void(int fd)> callback);

    /**
     * @brief Set disconnection callback
     * @param callback Function to call
     */
    void set_on_disconnect(std::function<void(int fd)> callback);

    /**
     * @brief Set message callback
     * @param callback Function to call with parsed message
     */
    void set_on_message(std::function<void(int fd, const json& msg)> callback);

    /**
     * @brief Send a message to a specific client
     * @param fd Client file descriptor
     * @param msg JSON message to send
     * @return true if sent successfully
     */
    bool send_to_client(int fd, const json& msg);

    /**
     * @brief Broadcast a message to all connected clients
     * @param msg JSON message to send
     */
    void broadcast(const json& msg);

    /**
     * @brief Get number of connected clients
     * @return Number of clients
     */
    size_t client_count() const;

private:
    std::string socket_path_;
    ToolRegistry& registry_;
    int server_fd_ = -1;
    std::atomic<bool> running_{false};
    std::thread accept_thread_;
    std::vector<std::thread> client_threads_;
    mutable std::mutex clients_mutex_;

    // Callbacks
    std::function<void(int fd)> on_connect_;
    std::function<void(int fd)> on_disconnect_;
    std::function<void(int fd, const json& msg)> on_message_;

    // Internal methods
    void accept_loop();
    void handle_client(int fd);
    bool setup_socket();
    void cleanup_socket();

    // Message handlers
    json handle_list_tools(const std::string& id);
    json handle_execute(const json& params, const std::string& id);
    json handle_ping();
    json handle_register_dynamic(const json& params);
    json handle_unregister(const json& params);

    // Process a single message from client
    json process_message(int fd, const json& msg);
};

/**
 * @brief Create and configure a socket server with default settings
 * @param socket_path Path to Unix socket
 * @param registry Tool registry
 * @return Configured socket server
 */
std::unique_ptr<SocketServer> create_socket_server(
    const std::string& socket_path,
    ToolRegistry& registry
);

} // namespace phone_service
