#include "get_location.h"

namespace phone_service {

ToolResult execute_get_location(const json& params) {
    std::string accuracy = "balanced";

    if (params.contains("accuracy")) {
        accuracy = params["accuracy"].get<std::string>();
    }

    // Validate accuracy
    if (accuracy != "high" && accuracy != "balanced" && accuracy != "low") {
        return ToolResult::fail("Invalid accuracy value: must be high, balanced, or low", -400);
    }

    // In a real implementation, this would use Android's LocationManager
    // or iOS's CoreLocation via JNI/NDKI
    // The accuracy determines GPS provider vs network provider

    // Simulated location data
    json location = {
        {"latitude", 37.7749},
        {"longitude", -122.4194},
        {"accuracy_meters", accuracy == "high" ? 5.0 : (accuracy == "balanced" ? 20.0 : 100.0)},
        {"altitude_meters", 10.5},
        {"speed_mps", 0.0},
        {"bearing_degrees", 0},
        {"provider", accuracy == "high" ? "gps" : "network"},
        {"timestamp", "2026-04-10T12:00:00Z"}
    };

    return ToolResult::ok(location);
}

void register_get_location_tool(ToolRegistry& registry) {
    Tool tool;
    tool.name = "get_location";
    tool.description = "Get current GPS location";
    tool.example = {{"accuracy", "high"}};

    tool.parameters["accuracy"] = ToolParameter{
        .type = "string",
        .description = "Location accuracy level (affects battery usage)",
        .required = false,
        .default_value = json("balanced"),
        .enum_values = {"high", "balanced", "low"}
    };

    registry.register_tool(tool, execute_get_location);
}

} // namespace phone_service
