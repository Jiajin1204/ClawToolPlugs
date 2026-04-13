#pragma once

#include "../tool_registry.h"
#include <string>

namespace phone_service {

/**
 * @brief Register search_images tool with its executor
 *
 * Tool: search_images
 * Description: Search for images by keyword from phone gallery
 * Parameters:
 *   - keyword (string, required): Keyword to search for
 *   - limit (integer, optional): Maximum results (default: 10)
 */
void register_search_images_tool(ToolRegistry& registry);

} // namespace phone_service
