#pragma once

#include "../tool_registry.h"

namespace phone_service {

/**
 * @brief Register send_notification tool with its executor
 *
 * Tool: send_notification
 * Description: Send a notification to the phone
 * Parameters:
 *   - title (string, required): Notification title
 *   - body (string, required): Notification body
 *   - urgency (string, optional): low, normal, high (default: normal)
 */
void register_send_notification_tool(ToolRegistry& registry);

} // namespace phone_service
