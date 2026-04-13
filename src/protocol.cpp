#include "protocol.h"
#include <iostream>

namespace phone_service {

std::optional<json> parse_message(const std::string& raw) {
    try {
        auto parsed = json::parse(raw);

        // Handle raw string messages (for convenience)
        if (parsed.is_string()) {
            return json::parse(parsed.get<std::string>());
        }

        return parsed;
    } catch (const json::parse_error& e) {
        std::cerr << "[protocol] Parse error: " << e.what() << std::endl;
        return std::nullopt;
    }
}

json make_tools_response(const json& tools, const std::string& id) {
    json msg = {
        {"type", MSG_TOOLS},
        {"tools", tools}
    };
    if (!id.empty()) {
        msg["id"] = id;
    }
    return msg;
}

json make_result_message(bool success, const json& data, const std::string& error, const std::string& id) {
    json msg = {
        {"type", MSG_RESULT},
        {"success", success}
    };

    if (!id.empty()) {
        msg["id"] = id;
    }
    if (success) {
        msg["data"] = data;
    } else {
        msg["error"] = error;
    }

    return msg;
}

json make_error_message(const std::string& error, int code) {
    return {
        {"type", MSG_ERROR},
        {"error", error},
        {"code", code}
    };
}

json make_pong_message() {
    return {
        {"type", MSG_PONG}
    };
}

json make_registered_message(int count) {
    return {
        {"type", MSG_REGISTERED},
        {"count", count}
    };
}

bool validate_message(const json& msg, const std::string& type) {
    if (!msg.is_object()) {
        return false;
    }

    auto it = msg.find("type");
    if (it == msg.end()) {
        return false;
    }

    return it.value().get<std::string>() == type;
}

} // namespace phone_service
