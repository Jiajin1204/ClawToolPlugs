/**
 * Type definitions for the phone-tools plugin
 */

/**
 * Configuration for the phone-tools plugin
 */
export interface PhoneToolsConfig {
  /** Path to the Unix socket for communication with phone service */
  socketPath: string;
  /** Delay in milliseconds between reconnection attempts */
  reconnectDelayMs: number;
  /** Connection timeout in milliseconds */
  connectTimeoutMs: number;
  /** Request timeout in milliseconds for tool execution */
  requestTimeoutMs: number;
}

/** Default configuration */
export const DEFAULT_CONFIG: PhoneToolsConfig = {
  socketPath: "/data/data/com.termux/files/home/phone-service/src/phone_service.sock",
  reconnectDelayMs: 2000,
  connectTimeoutMs: 5000,
  requestTimeoutMs: 30000,
};

/**
 * A tool parameter definition
 */
export interface ToolParameter {
  type: string;
  description: string;
  required?: boolean;
  default?: unknown;
  enum?: string[];
}

/**
 * A tool definition from the phone service
 */
export interface Tool {
  name: string;
  description: string;
  parameters: Record<string, ToolParameter>;
  example?: Record<string, unknown>;
}

/**
 * Protocol message types
 */
export const MessageType = {
  LIST_TOOLS: "list_tools",
  TOOLS: "tools",
  EXECUTE: "execute",
  RESULT: "result",
  ERROR: "error",
  PING: "ping",
  PONG: "pong",
  REGISTER_DYNAMIC: "register_dynamic",
  REGISTERED: "registered",
  UNREGISTER: "unregister",
} as const;

export type MessageTypeValue = (typeof MessageType)[keyof typeof MessageType];

/**
 * Connection state
 */
export type ConnectionState =
  | "disconnected"
  | "connecting"
  | "connected"
  | "reconnecting"
  | "error";

/**
 * Plugin logger interface
 */
export interface PluginLogger {
  debug?: (message: string) => void;
  info: (message: string) => void;
  warn: (message: string) => void;
  error: (message: string) => void;
}
