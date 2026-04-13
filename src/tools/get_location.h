#pragma once

#include "../tool_registry.h"

namespace phone_service {

/**
 * @brief Register get_location tool with its executor
 *
 * Tool: get_location
 * Description: Get current GPS location
 * Parameters:
 *   - accuracy (string, optional): high, balanced, low (default: balanced)
 */
void register_get_location_tool(ToolRegistry& registry);

} // namespace phone_service
