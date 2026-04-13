#pragma once

#include <string>
#include <optional>

// JSON library (nlohmann/json) - must be included before namespace
#include <nlohmann/json.hpp>

namespace phone_service {

// Import json type into phone_service namespace for convenience
using json = nlohmann::json;

// Message types
constexpr const char* MSG_LIST_TOOLS = "list_tools";
constexpr const char* MSG_TOOLS = "tools";
constexpr const char* MSG_EXECUTE = "execute";
constexpr const char* MSG_RESULT = "result";
constexpr const char* MSG_ERROR = "error";
constexpr const char* MSG_PING = "ping";
constexpr const char* MSG_PONG = "pong";
constexpr const char* MSG_REGISTER_DYNAMIC = "register_dynamic";
constexpr const char* MSG_REGISTERED = "registered";
constexpr const char* MSG_UNREGISTER = "unregister";

/**
 * @brief Parse a JSON string into a json object
 * @param raw Raw JSON string
 * @return Optional<json> parsed object, or std::nullopt on parse error
 */
std::optional<json> parse_message(const std::string& raw);

/**
 * @brief Create a tools list response message
 * @param tools Array of tool definitions
 * @return json message object
 */
json make_tools_response(const json& tools, const std::string& id = "");

/**
 * @brief Create an execution result message
 * @param success Whether execution succeeded
 * @param data Result data (for success)
 * @param error Error message (for failure)
 * @return json message object
 */
json make_result_message(bool success, const json& data, const std::string& error = "", const std::string& id = "");

/**
 * @brief Create an error message
 * @param error Error message
 * @param code Error code
 * @return json message object
 */
json make_error_message(const std::string& error, int code = -1);

/**
 * @brief Create a pong response for ping
 * @return json message object
 */
json make_pong_message();

/**
 * @brief Create a registered acknowledgment message
 * @param count Number of tools registered
 * @return json message object
 */
json make_registered_message(int count);

/**
 * @brief Validate a message has required fields
 * @param msg JSON message to validate
 * @param type Expected message type
 * @return true if valid
 */
bool validate_message(const json& msg, const std::string& type);

} // namespace phone_service
