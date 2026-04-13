/**
 * OpenClaw Phone Tools Plugin
 *
 * This plugin connects to a phone native service via Unix socket and exposes
 * the phone's native capabilities as tools to OpenClaw.
 */

import { definePluginEntry } from "openclaw/plugin-sdk/plugin-entry";
import type { OpenClawPluginApi, PluginLogger } from "openclaw/plugin-sdk";
import { Type, type TSchema } from "@sinclair/typebox";
import { SocketClient } from "./src/socket-client.js";
import type { PhoneToolsConfig, Tool, ToolParameter } from "./src/types.js";
import { DEFAULT_CONFIG } from "./src/types.js";

/**
 * Convert phone-service parameter format to TypeBox schema
 */
function paramToTypeBox(param: ToolParameter): TSchema {
  switch (param.type) {
    case "string":
      return Type.String({
        description: param.description,
        default: param.default as string | undefined,
      });
    case "integer":
      return Type.Number({
        description: param.description,
        default: param.default as number | undefined,
      });
    case "boolean":
      return Type.Boolean({
        description: param.description,
        default: param.default as boolean | undefined,
      });
    case "array":
      return Type.Array(Type.String(), {
        description: param.description,
      });
    default:
      return Type.String({ description: param.description });
  }
}

/**
 * Convert phone-service parameters to TypeBox schema
 */
function paramsToSchema(params: Record<string, ToolParameter>): TSchema {
  const properties: Record<string, TSchema> = {};
  const required: string[] = [];

  for (const [key, param] of Object.entries(params)) {
    properties[key] = paramToTypeBox(param);
    if (param.required) {
      required.push(key);
    }
  }

  return Type.Object(properties, { additionalProperties: false });
}

// Re-export types for external use
export type { PhoneToolsConfig, Tool, ConnectionState } from "./src/types.js";

/**
 * Plugin logger that prefixes messages with [phone-tools]
 */
function createLogger(api: OpenClawPluginApi): PluginLogger {
  return {
    debug: api.logger.debug ? (message: string) => { api.logger.debug?.(`[phone-tools] ${message}`); } : undefined,
    info: (message: string) => api.logger.info(`[phone-tools] ${message}`),
    warn: (message: string) => api.logger.warn(`[phone-tools] ${message}`),
    error: (message: string) => api.logger.error(`[phone-tools] ${message}`),
  };
}

/**
 * Validate and extract configuration from the plugin API
 */
function getConfig(api: OpenClawPluginApi): PhoneToolsConfig {
  const config = api.pluginConfig as Partial<PhoneToolsConfig> | undefined;

  return {
    socketPath: config?.socketPath ?? DEFAULT_CONFIG.socketPath,
    reconnectDelayMs: config?.reconnectDelayMs ?? DEFAULT_CONFIG.reconnectDelayMs,
    connectTimeoutMs: config?.connectTimeoutMs ?? DEFAULT_CONFIG.connectTimeoutMs,
    requestTimeoutMs: config?.requestTimeoutMs ?? DEFAULT_CONFIG.requestTimeoutMs,
  };
}

/**
 * Plugin entry point
 */
export default definePluginEntry({
  id: "phone-tools",
  name: "Phone Tools",
  description: "Access phone native tools via Unix socket connection",

  register: (api: OpenClawPluginApi) => {
    const logger = createLogger(api);
    const config = getConfig(api);

    logger.info("Registering phone-tools plugin");

    // Create socket client
    const socketClient = new SocketClient(config, logger);

    // Handle connection state changes
    socketClient.on("stateChanged", (state) => {
      logger.info(`Connection state changed: ${state}`);
    });

    socketClient.on("toolsUpdated", (tools) => {
      logger.info(`Tools updated: ${tools.length} tools available`);
    });

    // Connect and register tools
    socketClient.connect().then(async () => {
      logger.info("Connected to phone service");

      try {
        const tools = await socketClient.listTools();
        logger.info(`Received ${tools.length} tools from phone service`);

        // Register each tool
        for (const tool of tools) {
          logger.info(`Registering tool: phone_${tool.name}`);

          const executor = async (
            toolCallId: string,
            params: Record<string, unknown>,
            _signal?: AbortSignal
          ) => {
            logger.info(`[EXECUTE] phone_${tool.name} called with toolCallId=${toolCallId}, params=${JSON.stringify(params)}`);
            try {
              const result = await socketClient.executeTool(tool.name, params);
              logger.info(`[EXECUTE] phone_${tool.name} result: success=${result.success}`);
              if (result.success) {
                return {
                  content: [{ type: "text" as const, text: JSON.stringify(result.data, null, 2) }],
                  details: result.data,
                };
              } else {
                return {
                  content: [{ type: "text" as const, text: result.error || "Tool execution failed" }],
                  details: { error: result.error },
                };
              }
            } catch (err) {
              logger.error(`[EXECUTE] phone_${tool.name} exception: ${err}`);
              throw err;
            }
          };

          const paramSchema = paramsToSchema(tool.parameters);
          logger.info(`[REGISTER] phone_${tool.name} paramSchema: ${JSON.stringify(paramSchema)}`);

          const registration = api.registerTool(
            {
              name: `phone_${tool.name}`,
              label: `Phone: ${tool.name}`,
              description: `[Phone] ${tool.description}`,
              parameters: paramSchema,
              execute: executor,
            },
            { optional: true }
          );

          logger.info(`[REGISTER] phone_${tool.name} registered successfully`);
        }

        logger.info(`Phone tools plugin initialized with ${tools.length} tools`);
      } catch (error) {
        logger.error(`Failed to initialize phone tools: ${error}`);
      }
    }).catch((error) => {
      logger.error(`Failed to connect to phone service: ${error}`);
    });

    logger.info("Phone-tools plugin registration complete");
  },
});
