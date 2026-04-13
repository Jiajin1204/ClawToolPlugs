#include "send_notification.h"
#include <iostream>

namespace phone_service {

ToolResult execute_send_notification(const json& params) {
    std::string title;
    std::string body;
    std::string urgency = "normal";

    // Parse parameters
    if (!params.contains("title")) {
        return ToolResult::fail("Missing required parameter: title", -400);
    }
    title = params["title"].get<std::string>();

    if (!params.contains("body")) {
        return ToolResult::fail("Missing required parameter: body", -400);
    }
    body = params["body"].get<std::string>();

    if (params.contains("urgency")) {
        urgency = params["urgency"].get<std::string>();
    }

    // Validate urgency
    if (urgency != "low" && urgency != "normal" && urgency != "high") {
        return ToolResult::fail("Invalid urgency value: must be low, normal, or high", -400);
    }

    // In a real implementation, this would use Android's NotificationManager
    // or iOS's UNUserNotificationCenter via JNI/NDKI
    // For now, we just log and return success

    std::cout << "[notification] Title: " << title << std::endl;
    std::cout << "[notification] Body: " << body << std::endl;
    std::cout << "[notification] Urgency: " << urgency << std::endl;

    return ToolResult::ok(json{
        {"success", true},
        {"notification_id", "notif_12345"},
        {"title", title},
        {"urgency", urgency}
    });
}

void register_send_notification_tool(ToolRegistry& registry) {
    Tool tool;
    tool.name = "send_notification";
    tool.description = "Send a notification to the phone";
    tool.example = {
        {"title", "Meeting Reminder"},
        {"body", "You have a meeting in 5 minutes"},
        {"urgency", "high"}
    };

    tool.parameters["title"] = ToolParameter{
        .type = "string",
        .description = "Notification title",
        .required = true
    };

    tool.parameters["body"] = ToolParameter{
        .type = "string",
        .description = "Notification body text",
        .required = true
    };

    tool.parameters["urgency"] = ToolParameter{
        .type = "string",
        .description = "Notification urgency level",
        .required = false,
        .default_value = json("normal"),
        .enum_values = {"low", "normal", "high"}
    };

    registry.register_tool(tool, execute_send_notification);
}

} // namespace phone_service
