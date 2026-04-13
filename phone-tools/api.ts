/**
 * Public API exports for the phone-tools plugin
 */

// Re-export types
export type {
  PhoneToolsConfig,
  Tool,
  ToolParameter,
  ConnectionState,
  PluginLogger,
} from "./src/types.js";

// Re-export client classes
export { SocketClient } from "./src/socket-client.js";
export { ToolRegistry } from "./src/tool-registry.js";
