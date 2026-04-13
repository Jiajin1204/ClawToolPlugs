#include <iostream>
#include <memory>
#include <csignal>
#include <thread>
#include <chrono>

#include "server.h"
#include "tool_registry.h"
#include "tools/search_images.h"
#include "tools/get_device_info.h"
#include "tools/send_notification.h"
#include "tools/get_location.h"
#include "tools/install_app.h"

using namespace phone_service;

// Global server pointer for signal handler
std::unique_ptr<SocketServer> g_server;

// Signal handler for clean shutdown
void signal_handler(int signum) {
    std::cout << "\n[main] Received signal " << signum << ", shutting down..." << std::endl;

    if (g_server) {
        g_server->stop();
    }
}

void print_banner() {
    std::cout << "==============================================\n";
    std::cout << "       Phone Service v1.0.0\n";
    std::cout << "       Unix Socket Tool Server\n";
    std::cout << "==============================================\n";
}

void print_usage(const char* program_name) {
    std::cout << "\nUsage: " << program_name << " [OPTIONS]\n";
    std::cout << "\nOptions:\n";
    std::cout << "  -s, --socket PATH    Unix socket path (default: /tmp/phone_service.sock)\n";
    std::cout << "  -c, --config FILE    Tools config JSON file (default: tools.json)\n";
    std::cout << "  -h, --help           Show this help message\n";
    std::cout << "\n";
}

int main(int argc, char* argv[]) {
    print_banner();

    // Default configuration - use termux writable directory
    std::string socket_path = "/data/data/com.termux/files/home/phone_service.sock";
    std::string config_file = "tools.json";

    // Parse command line arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "-s" || arg == "--socket") {
            if (i + 1 < argc) {
                socket_path = argv[++i];
            } else {
                std::cerr << "Error: --socket requires an argument\n";
                return 1;
            }
        } else if (arg == "-c" || arg == "--config") {
            if (i + 1 < argc) {
                config_file = argv[++i];
            } else {
                std::cerr << "Error: --config requires an argument\n";
                return 1;
            }
        } else if (arg == "-h" || arg == "--help") {
            print_usage(argv[0]);
            return 0;
        } else {
            std::cerr << "Unknown option: " << arg << "\n";
            print_usage(argv[0]);
            return 1;
        }
    }

    // Set up signal handlers
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    // Initialize tool registry
    ToolRegistry& registry = get_global_registry();

    // Register built-in tools with their executors
    register_search_images_tool(registry);
    register_get_device_info_tool(registry);
    register_send_notification_tool(registry);
    register_get_location_tool(registry);
    register_install_app_tool(registry);

    std::cout << "[main] Registered " << registry.tool_count() << " built-in tools\n";

    // Load tools from config file
    if (!config_file.empty()) {
        if (registry.load_from_file(config_file)) {
            std::cout << "[main] Loaded tools from: " << config_file << "\n";
        } else {
            std::cout << "[main] Warning: Could not load tools from: " << config_file << "\n";
            std::cout << "[main] Continuing with built-in tools only\n";
        }
    }

    // Create and start server
    g_server = create_socket_server(socket_path, registry);

    std::cout << "[main] Starting server...\n";
    std::cout << "[main] Socket path: " << socket_path << "\n";
    std::cout << "[main] Press Ctrl+C to stop\n\n";

    if (!g_server->start()) {
        std::cerr << "[main] Failed to start server\n";
        return 1;
    }

    // Keep running until interrupted
    while (g_server->is_running()) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    std::cout << "[main] Server stopped\n";
    return 0;
}
