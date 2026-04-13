#include "tool_registry.h"
#include <fstream>
#include <iostream>
#include <algorithm>

namespace phone_service {

// ============================================================================
// ToolParameter
// ============================================================================

json ToolParameter::to_json() const {
    json j = {
        {"type", type},
        {"description", description}
    };

    if (required) {
        j["required"] = true;
    }

    if (!default_value.is_null()) {
        j["default"] = default_value;
    }

    if (!enum_values.empty()) {
        j["enum"] = enum_values;
    }

    return j;
}

// ============================================================================
// Tool
// ============================================================================

json Tool::to_json() const {
    json params_json = json::object();
    for (const auto& [name, param] : parameters) {
        params_json[name] = param.to_json();
    }

    json j = {
        {"name", name},
        {"description", description},
        {"parameters", params_json}
    };

    if (!example.is_null()) {
        j["example"] = example;
    }

    return j;
}

bool Tool::validate_params(const json& params) const {
    // Check required parameters
    for (const auto& [name, param] : parameters) {
        if (param.required) {
            auto it = params.find(name);
            if (it == params.end() || it.value().is_null()) {
                std::cerr << "[tool_registry] Missing required parameter: " << name << std::endl;
                return false;
            }
        }
    }

    // Type checking (basic)
    for (const auto& [name, value] : params.items()) {
        auto it = parameters.find(name);
        if (it == parameters.end()) {
            // Allow extra parameters for flexibility
            continue;
        }

        const auto& param_def = it->second;

        // Skip type validation if parameter not provided
        if (value.is_null()) {
            continue;
        }

        // Basic type checking
        bool type_valid = false;
        if (param_def.type == "string" && value.is_string()) {
            type_valid = true;
        } else if (param_def.type == "integer" && value.is_number_integer()) {
            type_valid = true;
        } else if (param_def.type == "number" && value.is_number()) {
            type_valid = true;
        } else if (param_def.type == "boolean" && value.is_boolean()) {
            type_valid = true;
        } else if (param_def.type == "object" && value.is_object()) {
            type_valid = true;
        } else if (param_def.type == "array" && value.is_array()) {
            type_valid = true;
        } else if (param_def.type == "string" && value.is_number()) {
            // Allow number for string type (will be converted)
            type_valid = true;
        }

        if (!type_valid) {
            std::cerr << "[tool_registry] Invalid type for parameter " << name
                      << ": expected " << param_def.type << std::endl;
            return false;
        }

        // Enum validation
        if (!param_def.enum_values.empty()) {
            std::string str_value;
            if (value.is_string()) {
                str_value = value.get<std::string>();
            } else {
                str_value = value.dump();
            }

            bool enum_valid = std::find(param_def.enum_values.begin(),
                                        param_def.enum_values.end(),
                                        str_value) != param_def.enum_values.end();
            if (!enum_valid) {
                std::cerr << "[tool_registry] Invalid enum value for " << name << std::endl;
                return false;
            }
        }
    }

    return true;
}

// ============================================================================
// ToolResult
// ============================================================================

ToolResult ToolResult::ok(const json& data) {
    return ToolResult{true, data, "", 0};
}

ToolResult ToolResult::fail(const std::string& err, int code) {
    return ToolResult{false, json::object(), err, code};
}

// ============================================================================
// ToolRegistry
// ============================================================================

ToolRegistry::ToolRegistry() = default;

ToolRegistry::~ToolRegistry() = default;

bool ToolRegistry::load_from_file(const std::string& file_path) {
    std::cout << "[tool_registry] Loading tools from: " << file_path << std::endl;

    try {
        std::ifstream file(file_path);
        if (!file.is_open()) {
            std::cerr << "[tool_registry] Failed to open file: " << file_path << std::endl;
            return false;
        }

        json tools_json;
        file >> tools_json;

        return load_from_json(tools_json);
    } catch (const std::exception& e) {
        std::cerr << "[tool_registry] Error loading tools: " << e.what() << std::endl;
        return false;
    }
}

bool ToolRegistry::load_from_json(const json& tools_json) {
    std::lock_guard<std::mutex> lock(mutex_);

    try {
        // Handle both direct array and wrapped object
        json tools_array;
        if (tools_json.is_array()) {
            tools_array = tools_json;
        } else if (tools_json.is_object() && tools_json.contains("tools")) {
            tools_array = tools_json["tools"];
        } else {
            std::cerr << "[tool_registry] Invalid tools JSON format" << std::endl;
            return false;
        }

        size_t loaded = 0;
        for (const auto& tool_def : tools_array) {
            if (load_single_tool(tool_def)) {
                loaded++;
            }
        }

        std::cout << "[tool_registry] Loaded " << loaded << " tools" << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "[tool_registry] Error parsing tools JSON: " << e.what() << std::endl;
        return false;
    }
}

bool ToolRegistry::load_single_tool(const json& tool_def) {
    try {
        Tool tool;
        tool.name = tool_def.at("name").get<std::string>();
        tool.description = tool_def.at("description").get<std::string>();

        if (tool_def.contains("example")) {
            tool.example = tool_def["example"];
        }

        // Parse parameters
        if (tool_def.contains("parameters")) {
            for (const auto& [name, param_def] : tool_def["parameters"].items()) {
                ToolParameter param;
                param.type = param_def.value("type", "string");
                param.description = param_def.value("description", "");
                param.required = param_def.value("required", false);

                if (param_def.contains("default")) {
                    param.default_value = param_def["default"];
                }

                if (param_def.contains("enum")) {
                    for (const auto& e : param_def["enum"]) {
                        param.enum_values.push_back(e.get<std::string>());
                    }
                }

                tool.parameters[name] = param;
            }
        }

        tools_[tool.name] = tool;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "[tool_registry] Error loading tool: " << e.what() << std::endl;
        return false;
    }
}

bool ToolRegistry::register_tool(const Tool& tool, ToolExecutor executor) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (tools_.count(tool.name)) {
        std::cerr << "[tool_registry] Tool already exists: " << tool.name << std::endl;
        return false;
    }

    tools_[tool.name] = tool;
    executors_[tool.name] = executor;
    dynamic_tool_names_.insert(tool.name);

    std::cout << "[tool_registry] Dynamically registered tool: " << tool.name << std::endl;
    return true;
}

bool ToolRegistry::unregister_tool(const std::string& tool_name) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto tool_it = tools_.find(tool_name);
    if (tool_it == tools_.end()) {
        return false;
    }

    // Only remove dynamically registered tools
    if (dynamic_tool_names_.count(tool_name)) {
        tools_.erase(tool_it);
        executors_.erase(tool_name);
        dynamic_tool_names_.erase(tool_name);
        std::cout << "[tool_registry] Unregistered tool: " << tool_name << std::endl;
        return true;
    }

    std::cerr << "[tool_registry] Cannot unregister static tool: " << tool_name << std::endl;
    return false;
}

std::vector<Tool> ToolRegistry::get_all_tools() const {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<Tool> result;
    for (const auto& [name, tool] : tools_) {
        result.push_back(tool);
    }
    return result;
}

std::optional<Tool> ToolRegistry::get_tool(const std::string& name) const {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = tools_.find(name);
    if (it != tools_.end()) {
        return it->second;
    }
    return std::nullopt;
}

bool ToolRegistry::has_tool(const std::string& name) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return tools_.count(name) > 0;
}

ToolResult ToolRegistry::execute_tool(const std::string& name, const json& params) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto tool_it = tools_.find(name);
    if (tool_it == tools_.end()) {
        return ToolResult::fail("Tool not found: " + name, -404);
    }

    const auto& tool = tool_it->second;

    // Validate parameters
    if (!tool.validate_params(params)) {
        return ToolResult::fail("Invalid parameters for tool: " + name, -400);
    }

    // Check if executor exists
    auto exec_it = executors_.find(name);
    if (exec_it == executors_.end()) {
        return ToolResult::fail("Tool has no executor: " + name, -500);
    }

    // Execute
    try {
        return exec_it->second(params);
    } catch (const std::exception& e) {
        return ToolResult::fail("Tool execution failed: " + std::string(e.what()), -500);
    }
}

size_t ToolRegistry::tool_count() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return tools_.size();
}

void ToolRegistry::clear_dynamic_tools() {
    std::lock_guard<std::mutex> lock(mutex_);

    for (const auto& name : dynamic_tool_names_) {
        tools_.erase(name);
        executors_.erase(name);
    }
    dynamic_tool_names_.clear();

    std::cout << "[tool_registry] Cleared all dynamic tools" << std::endl;
}

std::vector<std::string> ToolRegistry::get_tool_names() const {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<std::string> names;
    for (const auto& [name, _] : tools_) {
        names.push_back(name);
    }
    return names;
}

// ============================================================================
// Global registry
// ============================================================================

ToolRegistry& get_global_registry() {
    static ToolRegistry registry;
    return registry;
}

} // namespace phone_service
