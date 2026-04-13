/**
 * Unix Socket Client for communicating with the phone native service
 */

import { createConnection, type Socket } from "net";
import { EventEmitter } from "events";
import type {
  PhoneToolsConfig,
  ConnectionState,
  PluginLogger,
  Tool,
} from "./types.js";
import { DEFAULT_CONFIG, MessageType as MT } from "./types.js";

/**
 * Result of a tool execution request
 */
export interface ToolRequestResult {
  success: boolean;
  data?: unknown;
  error?: string;
}

/**
 * Socket client for phone service communication
 */
export class SocketClient extends EventEmitter {
  private config: PhoneToolsConfig;
  private socket: Socket | null = null;
  private state: ConnectionState = "disconnected";
  private reconnectTimer: ReturnType<typeof setTimeout> | null = null;
  private pingTimer: ReturnType<typeof setInterval> | null = null;
  private pendingRequests: Map<string, {
    resolve: (result: ToolRequestResult) => void;
    reject: (error: Error) => void;
    timer: ReturnType<typeof setTimeout>;
  }> = new Map();
  private messageBuffer: string = "";
  private logger: PluginLogger;
  private requestIdCounter: number = 0;

  constructor(config: Partial<PhoneToolsConfig> = {}, logger: PluginLogger) {
    super();
    this.config = { ...DEFAULT_CONFIG, ...config };
    this.logger = logger;
  }

  /**
   * Get current connection state
   */
  getState(): ConnectionState {
    return this.state;
  }

  /**
   * Check if connected
   */
  isConnected(): boolean {
    return this.state === "connected" && this.socket !== null;
  }

  /**
   * Connect to the phone service
   */
  async connect(): Promise<void> {
    if (this.state === "connected" || this.state === "connecting") {
      this.logger.debug?.("Already connected or connecting");
      return;
    }

    this.setState("connecting");
    this.logger.info(`Connecting to phone service at ${this.config.socketPath}`);

    return new Promise((resolve, reject) => {
      this.socket = createConnection({ path: this.config.socketPath });

      // Connection timeout
      const timeout = setTimeout(() => {
        if (this.state === "connecting") {
          this.socket?.destroy();
          reject(new Error(`Connection timeout after ${this.config.connectTimeoutMs}ms`));
        }
      }, this.config.connectTimeoutMs);

      this.socket.on("connect", () => {
        clearTimeout(timeout);
        this.logger.info("Connected to phone service");
        this.setState("connected");
        this.startPingTimer();
        resolve();
      });

      this.socket.on("data", (data: Buffer) => {
        this.handleData(data);
      });

      this.socket.on("error", (err: Error) => {
        clearTimeout(timeout);
        this.logger.error(`Socket error: ${err.message}`);

        if (this.state === "connecting") {
          reject(err);
        }

        this.handleDisconnect();
      });

      this.socket.on("close", () => {
        this.logger.debug?.("Socket closed");
        this.handleDisconnect();
      });

      this.socket.on("end", () => {
        this.logger.debug?.("Socket ended");
        this.handleDisconnect();
      });
    });
  }

  /**
   * Disconnect from the phone service
   */
  disconnect(): void {
    this.logger.info("Disconnecting from phone service");
    this.stopPingTimer();
    this.cancelReconnect();

    if (this.reconnectTimer) {
      clearTimeout(this.reconnectTimer);
      this.reconnectTimer = null;
    }

    if (this.socket) {
      this.socket.destroy();
      this.socket = null;
    }

    // Reject pending requests
    for (const [, pending] of this.pendingRequests) {
      clearTimeout(pending.timer);
      pending.reject(new Error("Connection closed"));
    }
    this.pendingRequests.clear();

    this.setState("disconnected");
  }

  /**
   * Send a request and wait for response
   */
  async sendRequest(message: Record<string, unknown>): Promise<unknown> {
    if (!this.isConnected()) {
      throw new Error("Not connected to phone service");
    }

    const requestId = `req_${++this.requestIdCounter}`;
    const fullMessage = { ...message, id: requestId };

    return new Promise((resolve, reject) => {
      const timeout = setTimeout(() => {
        this.pendingRequests.delete(requestId);
        reject(new Error(`Request timeout after ${this.config.requestTimeoutMs}ms`));
      }, this.config.requestTimeoutMs);

      const pendingResult: ToolRequestResult = { success: false };

      this.pendingRequests.set(requestId, {
        resolve: (result: ToolRequestResult) => {
          clearTimeout(timeout);
          resolve(result);
        },
        reject,
        timer: timeout,
      });

      const messageStr = JSON.stringify(fullMessage) + "\n";
      const socketRef = this.socket;
      if (socketRef) {
        socketRef.write(messageStr, () => {
          // Write completed, response will come asynchronously
        });
      }
    });
  }

  /**
   * List all available tools
   */
  async listTools(): Promise<Tool[]> {
    this.logger.debug?.("Requesting tool list");

    const response = await this.sendRequest({ type: MT.LIST_TOOLS }) as Record<string, unknown>;

    if (response.type !== MT.TOOLS) {
      throw new Error(`Unexpected response type: ${response.type}`);
    }

    const tools = (response as { tools: Tool[] }).tools;
    this.logger.info(`Received ${tools.length} tools from phone service`);
    return tools;
  }

  /**
   * Execute a tool
   */
  async executeTool(toolName: string, params: Record<string, unknown> = {}): Promise<ToolRequestResult> {
    this.logger.debug?.(`Executing tool: ${toolName}`);

    const response = await this.sendRequest({
      type: MT.EXECUTE,
      tool: toolName,
      params,
    }) as Record<string, unknown>;

    if (response.type === MT.ERROR) {
      const errResp = response as { error: string };
      this.logger.error(`Tool execution error: ${errResp.error}`);
      return {
        success: false,
        error: errResp.error,
      };
    }

    if (response.type === MT.RESULT) {
      const resultResp = response as { success: boolean; data?: unknown; error?: string };
      if (resultResp.success) {
        this.logger.debug?.(`Tool ${toolName} executed successfully`);
        return {
          success: true,
          data: resultResp.data,
        };
      } else {
        this.logger.warn(`Tool ${toolName} failed: ${resultResp.error}`);
        return {
          success: false,
          error: resultResp.error,
        };
      }
    }

    return {
      success: false,
      error: `Unexpected response type: ${response.type}`,
    };
  }

  /**
   * Send ping and wait for pong
   */
  async ping(): Promise<boolean> {
    try {
      const response = await this.sendRequest({ type: MT.PING }) as Record<string, unknown>;
      return response.type === MT.PONG;
    } catch {
      return false;
    }
  }

  private handleData(data: Buffer): void {
    this.messageBuffer += data.toString();

    // Process complete messages (newline-delimited)
    const lines = this.messageBuffer.split("\n");
    this.messageBuffer = lines.pop() || "";

    for (const line of lines) {
      if (line.trim()) {
        this.handleMessage(line);
      }
    }
  }

  private handleMessage(rawMessage: string): void {
    try {
      const message = JSON.parse(rawMessage) as Record<string, unknown>;

      // Check if this is a response to a pending request
      if ("id" in message && message.id && this.pendingRequests.has(message.id as string)) {
        const pending = this.pendingRequests.get(message.id as string)!;
        this.pendingRequests.delete(message.id as string);
        clearTimeout(pending.timer);

        const msgType = message.type as string;

        // Handle different response types
        if (msgType === MT.RESULT) {
          const result = message as { success: boolean; data?: unknown; error?: string };
          pending.resolve({
            success: result.success,
            data: result.data,
            error: result.error,
          });
        } else if (msgType === MT.ERROR) {
          const error = message as { error: string; code?: number };
          pending.resolve({
            success: false,
            error: `${error.error} (code: ${error.code})`,
          });
        } else if (msgType === MT.TOOLS) {
          pending.resolve({ success: true, data: message });
        } else if (msgType === MT.PONG) {
          pending.resolve({ success: true });
        } else {
          pending.resolve({ success: false, error: `Unknown response type: ${msgType}` });
        }
        return;
      }

      // Emit the message for event listeners
      this.emit("message", message);
    } catch (err) {
      this.logger.error(`Failed to parse message: ${err}`);
    }
  }

  private handleDisconnect(): void {
    this.stopPingTimer();

    const wasConnected = this.state === "connected" || this.state === "connecting";

    if (this.socket) {
      this.socket.destroy();
      this.socket = null;
    }

    if (wasConnected && this.state !== "disconnected") {
      this.logger.warn("Connection lost, attempting to reconnect...");
      this.setState("reconnecting");
      this.scheduleReconnect();
    } else if (this.state !== "disconnected") {
      this.setState("error");
    }
  }

  private scheduleReconnect(): void {
    this.cancelReconnect();

    this.reconnectTimer = setTimeout(async () => {
      if (this.state !== "reconnecting") {
        return;
      }

      try {
        await this.connect();
        // Re-request tools on reconnect
        const tools = await this.listTools();
        this.emit("toolsUpdated", tools);
      } catch (err) {
        this.logger.error(`Reconnect failed: ${err}`);
        this.scheduleReconnect();
      }
    }, this.config.reconnectDelayMs);
  }

  private cancelReconnect(): void {
    if (this.reconnectTimer) {
      clearTimeout(this.reconnectTimer);
      this.reconnectTimer = null;
    }
  }

  private startPingTimer(): void {
    this.stopPingTimer();

    // Send ping every 30 seconds to keep connection alive
    this.pingTimer = setInterval(async () => {
      if (this.isConnected()) {
        const ok = await this.ping();
        if (!ok) {
          this.logger.warn("Ping failed, connection may be dead");
        }
      }
    }, 30000);
  }

  private stopPingTimer(): void {
    if (this.pingTimer) {
      clearInterval(this.pingTimer);
      this.pingTimer = null;
    }
  }

  private setState(state: ConnectionState): void {
    if (this.state !== state) {
      this.state = state;
      this.emit("stateChanged", state);
    }
  }
}
