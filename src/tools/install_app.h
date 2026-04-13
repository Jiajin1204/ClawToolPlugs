#pragma once

#include "../tool_registry.h"

namespace phone_service {

/**
 * @brief App installer interface (Strategy Pattern)
 *
 * Defines the interface for different app installation methods.
 * This allows flexible implementation of various installation approaches.
 */
class IAppInstaller {
public:
    virtual ~IAppInstaller() = default;

    /**
     * @brief Install an app from the given source
     * @param apk_path Path to the APK file
     * @param params Additional installation parameters
     * @return ToolResult containing installation result
     */
    virtual ToolResult install(const std::string& apk_path, const json& params) = 0;

    /**
     * @brief Get the installer name
     * @return Installer name
     */
    virtual std::string get_name() const = 0;

    /**
     * @brief Check if this installer is available on the system
     * @return true if available
     */
    virtual bool is_available() const = 0;
};

/**
 * @brief Package Manager installer (default Android method)
 *
 * Uses the 'pm' command to install APKs.
 */
class PmInstaller : public IAppInstaller {
public:
    ToolResult install(const std::string& apk_path, const json& params) override;
    std::string get_name() const override { return "pm"; }
    bool is_available() const override;

private:
    ToolResult execute_pm_command(const std::vector<std::string>& args);
};

/**
 * @brief Shell installer (alternative method)
 *
 * Uses 'cmd' command for package installation on newer Android versions.
 */
class CmdInstaller : public IAppInstaller {
public:
    ToolResult install(const std::string& apk_path, const json& params) override;
    std::string get_name() const override { return "cmd"; }
    bool is_available() const override;
};

/**
 * @brief App Installer Factory (Factory Pattern)
 *
 * Creates appropriate installer based on system capabilities.
 */
class AppInstallerFactory {
public:
    /**
     * @brief Get the best available installer
     * @return Pointer to installer, or nullptr if none available
     */
    static std::unique_ptr<IAppInstaller> create();

    /**
     * @brief Register a custom installer
     * @param installer Unique pointer to installer
     */
    static void register_installer(std::unique_ptr<IAppInstaller> installer);

private:
    static std::vector<std::unique_ptr<IAppInstaller>>& get_installers();
};

/**
 * @brief Register install_app tool with its executor
 *
 * Tool: install_app
 * Description: Install an application package (APK) on the phone
 * Parameters:
 *   - apk_path (string, required): Path to the APK file
 *   - method (string, optional): Installation method - "pm" or "cmd" (auto-detect by default)
 */
void register_install_app_tool(ToolRegistry& registry);

} // namespace phone_service
