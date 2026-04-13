#include "search_images.h"
#include <iostream>
#include <random>
#include <algorithm>

namespace phone_service {

// Simulated image database (in production, this would query the phone's gallery)
struct SimulatedImage {
    std::string filename;
    std::string path;
    std::vector<std::string> tags;
    int size_kb;
};

static const std::vector<SimulatedImage> k_simulated_images = {
    {"sunset_beach.jpg", "/sdcard/DCIM/sunset_beach.jpg", {"sunset", "beach", "ocean", "landscape"}, 2048},
    {"mountain_view.jpg", "/sdcard/DCIM/mountain_view.jpg", {"mountain", "nature", "landscape", "hiking"}, 1536},
    {"city_night.jpg", "/sdcard/DCIM/city_night.jpg", {"city", "night", "urban", "lights"}, 2048},
    {"cat_portrait.jpg", "/sdcard/Pictures/cat_portrait.jpg", {"cat", "animal", "pet", "portrait"}, 1024},
    {"dog_play.jpg", "/sdcard/Pictures/dog_play.jpg", {"dog", "animal", "pet", "play"}, 1280},
    {"food_pasta.jpg", "/sdcard/Pictures/food_pasta.jpg", {"food", "pasta", "meal", "italian"}, 896},
    {"coffee_morning.jpg", "/sdcard/Pictures/coffee_morning.jpg", {"coffee", "morning", "drink", "breakfast"}, 512},
    {"travel_eiffel.jpg", "/sdcard/Travel/travel_eiffel.jpg", {"travel", "paris", "france", "landmark"}, 3072},
    {"family_reunion.jpg", "/sdcard/Family/family_reunion.jpg", {"family", "reunion", "people", "event"}, 2560},
    {"sunset_desert.jpg", "/sdcard/DCIM/sunset_desert.jpg", {"sunset", "desert", "landscape", "nature"}, 1792},
};

ToolResult execute_search_images(const json& params) {
    std::string keyword;
    int limit = 10;

    // Parse parameters
    if (params.contains("keyword")) {
        keyword = params["keyword"].get<std::string>();
    } else {
        return ToolResult::fail("Missing required parameter: keyword", -400);
    }

    if (params.contains("limit")) {
        limit = params["limit"].get<int>();
    }

    // Normalize keyword to lowercase for searching
    std::string search_term = keyword;
    std::transform(search_term.begin(), search_term.end(), search_term.begin(), ::tolower);

    // Search through simulated images
    std::vector<json> results;
    for (const auto& img : k_simulated_images) {
        bool matches = false;

        // Check filename
        std::string filename_lower = img.filename;
        std::transform(filename_lower.begin(), filename_lower.end(), filename_lower.begin(), ::tolower);
        if (filename_lower.find(search_term) != std::string::npos) {
            matches = true;
        }

        // Check tags
        if (!matches) {
            for (const auto& tag : img.tags) {
                std::string tag_lower = tag;
                std::transform(tag_lower.begin(), tag_lower.end(), tag_lower.begin(), ::tolower);
                if (tag_lower.find(search_term) != std::string::npos) {
                    matches = true;
                    break;
                }
            }
        }

        if (matches) {
            json result_item = {
                {"filename", img.filename},
                {"path", img.path},
                {"tags", img.tags},
                {"size_kb", img.size_kb},
                {"type", "image/jpeg"}  // Simplified, assume JPEG
            };
            results.push_back(result_item);

            if (static_cast<int>(results.size()) >= limit) {
                break;
            }
        }
    }

    return ToolResult::ok(json{
        {"keyword", keyword},
        {"count", static_cast<int>(results.size())},
        {"images", results}
    });
}

void register_search_images_tool(ToolRegistry& registry) {
    Tool tool;
    tool.name = "search_images";
    tool.description = "Search for images by keyword from phone gallery";
    tool.example = {{"keyword", "sunset"}, {"limit", 5}};

    // Define parameters
    tool.parameters["keyword"] = ToolParameter{
        .type = "string",
        .description = "Keyword to search for in image filenames or tags",
        .required = true
    };

    tool.parameters["limit"] = ToolParameter{
        .type = "integer",
        .description = "Maximum number of results to return",
        .required = false,
        .default_value = json(10)
    };

    registry.register_tool(tool, execute_search_images);
}

} // namespace phone_service
