#include "get_device_info.h"

namespace phone_service {

ToolResult execute_get_device_info(const json& params) {
    // In a real implementation, this would read from:
    // - /sys/class/power_supply/battery/* for battery info
    // - statfs() for storage info
    // - netlink sockets for network info

    // Simulated data for demonstration
    json device_info = {
        {"battery", {
            {"level", 75},
            {"is_charging", false},
            {"status", "discharging"},
            {"temperature_celsius", 28.5}
        }},
        {"storage", {
            {"total_gb", 128},
            {"used_gb", 64},
            {"available_gb", 64},
            {"percentage_used", 50}
        }},
        {"memory", {
            {"total_mb", 8192},
            {"used_mb", 4096},
            {"available_mb", 4096},
            {"percentage_used", 50}
        }},
        {"network", {
            {"wifi_connected", true},
            {"wifi_ssid", "MyHomeNetwork"},
            {"wifi_signal_strength", -55},  // dBm
            {"mobile_connected", false},
            {"ip_address", "192.168.1.100"}
        }},
        {"system", {
            {"os", "Android 14"},
            {"manufacturer", "ExamplePhone"},
            {"model", "ExamplePhone Pro"},
            {"uptime_seconds", 86400}  // 24 hours
        }}
    };

    return ToolResult::ok(device_info);
}

void register_get_device_info_tool(ToolRegistry& registry) {
    Tool tool;
    tool.name = "get_device_info";
    tool.description = "Get device information such as battery, storage, network status";
    tool.example = json::object();  // No parameters

    // This tool has no parameters
    registry.register_tool(tool, execute_get_device_info);
}

} // namespace phone_service
