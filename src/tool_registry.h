#pragma once

#include <string>
#include <vector>
#include <map>
#include <set>
#include <functional>
#include <mutex>
#include <optional>

#include "protocol.h"

namespace phone_service {

/**
 * @brief Represents a tool parameter
 */
struct ToolParameter {
    std::string type;           // "string", "integer", "boolean", "object", "array"
    std::string description;    // Human-readable description
    bool required = false;      // Whether this parameter is required
    json default_value;         // Default value (if not required)
    std::vector<std::string> enum_values;  // For enum types

    json to_json() const;
};

/**
 * @brief Represents a tool definition
 */
struct Tool {
    std::string name;           // Unique tool name
    std::string description;    // Human-readable description
    std::map<std::string, ToolParameter> parameters;  // Parameter name -> definition
    json example;              // Example usage

    json to_json() const;
    bool validate_params(const json& params) const;
};

/**
 * @brief Tool execution context
 */
struct ToolContext {
    std::string tool_name;      // Name of tool being executed
    json params;                // Parameters passed
};

/**
 * @brief Tool execution result
 */
struct ToolResult {
    bool success;
    json data;                 // Result data (if success)
    std::string error_message; // Error message (if failed)
    int error_code = 0;

    static ToolResult ok(const json& data);
    static ToolResult fail(const std::string& err, int code = -1);
};

/**
 * @brief Tool executor function type
 */
using ToolExecutor = std::function<ToolResult(const json& params)>;

/**
 * @brief Tool registry for managing available tools
 *
 * Supports both static (JSON config) and dynamic (runtime) tool registration.
 * Thread-safe for concurrent access.
 */
class ToolRegistry {
public:
    ToolRegistry();
    ~ToolRegistry();

    /**
     * @brief Load tools from a JSON file
     * @param file_path Path to tools.json file
     * @return true if loaded successfully
     */
    bool load_from_file(const std::string& file_path);

    /**
     * @brief Load tools from a JSON object
     * @param tools_json JSON array of tool definitions
     * @return true if loaded successfully
     */
    bool load_from_json(const json& tools_json);

    /**
     * @brief Register a tool dynamically at runtime
     * @param tool Tool definition
     * @param executor Function to execute the tool
     * @return true if registered successfully
     */
    bool register_tool(const Tool& tool, ToolExecutor executor);

    /**
     * @brief Unregister a tool by name
     * @param tool_name Name of tool to remove
     * @return true if tool was found and removed
     */
    bool unregister_tool(const std::string& tool_name);

    /**
     * @brief Get all registered tools
     * @return Vector of tool definitions
     */
    std::vector<Tool> get_all_tools() const;

    /**
     * @brief Get a specific tool by name
     * @param name Tool name
     * @return Tool if found, std::nullopt otherwise
     */
    std::optional<Tool> get_tool(const std::string& name) const;

    /**
     * @brief Check if a tool exists
     * @param name Tool name
     * @return true if tool exists
     */
    bool has_tool(const std::string& name) const;

    /**
     * @brief Execute a tool by name
     * @param name Tool name
     * @param params Parameters to pass
     * @return Tool execution result
     */
    ToolResult execute_tool(const std::string& name, const json& params);

    /**
     * @brief Get the number of registered tools
     * @return Number of tools
     */
    size_t tool_count() const;

    /**
     * @brief Clear all dynamically registered tools (keeps static ones)
     */
    void clear_dynamic_tools();

    /**
     * @brief Get list of tool names
     * @return Vector of tool names
     */
    std::vector<std::string> get_tool_names() const;

private:
    std::map<std::string, Tool> tools_;                      // Tool definitions
    std::map<std::string, ToolExecutor> executors_;         // Tool executors
    std::set<std::string> dynamic_tool_names_;              // Track dynamically added tools
    mutable std::mutex mutex_;                               // Thread safety

    bool load_single_tool(const json& tool_def);
};

/**
 * @brief Create the global tool registry instance
 */
ToolRegistry& get_global_registry();

} // namespace phone_service
