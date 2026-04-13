#include "install_app.h"
#include <fstream>
#include <cstdlib>
#include <array>
#include <memory>
#include <iostream>

namespace phone_service {

// ============================================================================
// PmInstaller Implementation
// ============================================================================

bool PmInstaller::is_available() const {
    return system("which pm > /dev/null 2>&1") == 0;
}

ToolResult PmInstaller::execute_pm_command(const std::vector<std::string>& args) {
    // Build command string
    std::string cmd = "pm";
    for (const auto& arg : args) {
        cmd += " " + arg;
    }
    cmd += " 2>&1";

    // Execute and capture output
    std::array<char, 256> buffer;
    std::string result;
    FILE* pipe = popen(cmd.c_str(), "r");

    if (!pipe) {
        return ToolResult::fail("Failed to execute pm command", -500);
    }

    while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
        result += buffer.data();
    }

    int status = pclose(pipe);

    if (status == 0) {
        return ToolResult::ok(json{
            {"success", true},
            {"message", "App installed successfully"},
            {"installer", get_name()},
            {"output", result}
        });
    } else {
        return ToolResult::fail("Installation failed: " + result, status);
    }
}

ToolResult PmInstaller::install(const std::string& apk_path, const json& params) {
    // Check if APK file exists
    std::ifstream file(apk_path);
    if (!file.good()) {
        return ToolResult::fail("APK file not found: " + apk_path, -404);
    }

    // Build pm install command
    std::vector<std::string> pm_args;

    // Handle installation flags from params
    bool allow_version_downgrade = params.value("allow_version_downgrade", false);
    bool grant_runtime_permissions = params.value("grant_runtime_permissions", true);

    if (allow_version_downgrade) {
        pm_args.push_back("-d");
    }
    if (grant_runtime_permissions) {
        pm_args.push_back("-g");
    }

    pm_args.push_back("-r");  // Replace existing app
    pm_args.push_back(apk_path);

    return execute_pm_command(pm_args);
}

// ============================================================================
// CmdInstaller Implementation
// ============================================================================

bool CmdInstaller::is_available() const {
    return system("which cmd > /dev/null 2>&1") == 0;
}

ToolResult CmdInstaller::install(const std::string& apk_path, const json& params) {
    // Check if APK file exists
    std::ifstream file(apk_path);
    if (!file.good()) {
        return ToolResult::fail("APK file not found: " + apk_path, -404);
    }

    // Build cmd package install command
    std::string cmd = "cmd package install";
    if (params.value("allow_version_downgrade", false)) {
        cmd += " -d";
    }
    cmd += " -r " + apk_path + " 2>&1";

    std::array<char, 256> buffer;
    std::string result;
    FILE* pipe = popen(cmd.c_str(), "r");

    if (!pipe) {
        return ToolResult::fail("Failed to execute cmd command", -500);
    }

    while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
        result += buffer.data();
    }

    int status = pclose(pipe);

    if (status == 0) {
        return ToolResult::ok(json{
            {"success", true},
            {"message", "App installed successfully"},
            {"installer", get_name()},
            {"output", result}
        });
    } else {
        return ToolResult::fail("Installation failed: " + result, status);
    }
}

// ============================================================================
// AppInstallerFactory Implementation
// ============================================================================

std::vector<std::unique_ptr<IAppInstaller>>& AppInstallerFactory::get_installers() {
    static std::vector<std::unique_ptr<IAppInstaller>> installers;
    return installers;
}

std::unique_ptr<IAppInstaller> AppInstallerFactory::create() {
    auto& installers = get_installers();

    // Try registered installers first
    for (auto& installer : installers) {
        if (installer->is_available()) {
            return std::move(installer);
        }
    }

    // Try built-in installers
    auto pm = std::make_unique<PmInstaller>();
    if (pm->is_available()) {
        return pm;
    }

    auto cmd = std::make_unique<CmdInstaller>();
    if (cmd->is_available()) {
        return cmd;
    }

    return nullptr;
}

void AppInstallerFactory::register_installer(std::unique_ptr<IAppInstaller> installer) {
    get_installers().push_back(std::move(installer));
}

// ============================================================================
// Tool Registration
// ============================================================================

static ToolResult execute_install_app(const json& params) {
    std::string apk_path;

    std::cout << "[install_app] Received request, params: " << params.dump() << std::endl;

    // Parse required parameter: apk_path
    if (!params.contains("apk_path")) {
        std::cout << "[install_app] ERROR: Missing required parameter: apk_path" << std::endl;
        return ToolResult::fail("Missing required parameter: apk_path", -400);
    }
    apk_path = params["apk_path"].get<std::string>();

    if (apk_path.empty()) {
        std::cout << "[install_app] ERROR: apk_path is empty" << std::endl;
        return ToolResult::fail("apk_path cannot be empty", -400);
    }

    std::cout << "[install_app] APK path: " << apk_path << std::endl;

    // Get installer (auto-detect or use specified)
    std::string method = params.value("method", "auto");
    std::unique_ptr<IAppInstaller> installer;

    if (method == "auto") {
        std::cout << "[install_app] Auto-detecting installer..." << std::endl;
        installer = AppInstallerFactory::create();
    } else if (method == "pm") {
        std::cout << "[install_app] Using pm installer (user specified)" << std::endl;
        installer = std::make_unique<PmInstaller>();
        if (!installer->is_available()) {
            std::cout << "[install_app] ERROR: pm installer not available" << std::endl;
            return ToolResult::fail("pm installer not available on this system", -501);
        }
    } else if (method == "cmd") {
        std::cout << "[install_app] Using cmd installer (user specified)" << std::endl;
        installer = std::make_unique<CmdInstaller>();
        if (!installer->is_available()) {
            std::cout << "[install_app] ERROR: cmd installer not available" << std::endl;
            return ToolResult::fail("cmd installer not available on this system", -501);
        }
    } else {
        std::cout << "[install_app] ERROR: Unknown method: " << method << std::endl;
        return ToolResult::fail("Unknown installation method: " + method + ". Use 'pm', 'cmd', or 'auto'", -400);
    }

    if (!installer) {
        std::cout << "[install_app] ERROR: No suitable installer found" << std::endl;
        return ToolResult::fail("No suitable installer found on this system", -501);
    }

    // Perform installation
    std::cout << "[install_app] Installing APK: " << apk_path << " using " << installer->get_name() << std::endl;
    ToolResult result = installer->install(apk_path, params);
    std::cout << "[install_app] Result: success=" << result.success << ", error=" << result.error_message << std::endl;
    return result;
}

void register_install_app_tool(ToolRegistry& registry) {
    Tool tool;
    tool.name = "install_app";
    tool.description = "Install an application package (APK) on the phone";
    tool.example = {
        {"apk_path", "/sdcard/Download/myapp.apk"},
        {"method", "auto"}
    };

    // Define parameters
    tool.parameters["apk_path"] = ToolParameter{
        .type = "string",
        .description = "Path to the APK file to install",
        .required = true
    };

    tool.parameters["method"] = ToolParameter{
        .type = "string",
        .description = "Installation method: 'auto' (default), 'pm', or 'cmd'",
        .required = false,
        .default_value = json("auto")
    };

    tool.parameters["allow_version_downgrade"] = ToolParameter{
        .type = "boolean",
        .description = "Allow version downgrade (default: false)",
        .required = false,
        .default_value = json(false)
    };

    tool.parameters["grant_runtime_permissions"] = ToolParameter{
        .type = "boolean",
        .description = "Grant runtime permissions during install (default: true, Android 6.0+)",
        .required = false,
        .default_value = json(true)
    };

    registry.register_tool(tool, execute_install_app);
}

} // namespace phone_service
