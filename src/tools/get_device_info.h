#pragma once

#include "../tool_registry.h"

namespace phone_service {

/**
 * @brief Register get_device_info tool with its executor
 *
 * Tool: get_device_info
 * Description: Get device information such as battery, storage, network status
 * Parameters: None
 */
void register_get_device_info_tool(ToolRegistry& registry);

} // namespace phone_service
