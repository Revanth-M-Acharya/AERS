import { EventLogEntry, IntersectionState, AmbulanceTelemetry } from '../types';

export type WebSocketStatus = 'CONNECTED' | 'CONNECTING' | 'DISCONNECTED';

type MessageListener = (data: any) => void;

class WebSocketService {
  private socket: WebSocket | null = null;
  private url: string = 'ws://localhost:9001';
  private listeners: Set<MessageListener> = new Set();
  private statusListeners: Set<(status: WebSocketStatus) => void> = new Set();
  private status: WebSocketStatus = 'DISCONNECTED';
  private reconnectTimer: number | null = null;
  private autoReconnect: boolean = true;

  constructor() {
    // Read from window storage if configured
    const savedUrl = localStorage.getItem('aegis_ws_url');
    if (savedUrl) {
      this.url = savedUrl;
    }
  }

  public setUrl(url: string) {
    this.url = url;
    localStorage.setItem('aegis_ws_url', url);
    if (this.socket) {
      this.disconnect();
      this.connect();
    }
  }

  public getUrl(): string {
    return this.url;
  }

  public getStatus(): WebSocketStatus {
    return this.status;
  }

  public connect() {
    if (this.socket && (this.socket.readyState === WebSocket.OPEN || this.socket.readyState === WebSocket.CONNECTING)) {
      return;
    }

    this.setStatus('CONNECTING');

    try {
      this.socket = new WebSocket(this.url);

      this.socket.onopen = () => {
        this.setStatus('CONNECTED');
        if (this.reconnectTimer) {
          clearTimeout(this.reconnectTimer);
          this.reconnectTimer = null;
        }
      };

      this.socket.onmessage = (event) => {
        try {
          const parsed = JSON.parse(event.data);
          this.notifyListeners(parsed);
        } catch (e) {
          console.warn('[AEGIS WS] Failed to parse message JSON:', event.data);
        }
      };

      this.socket.onclose = () => {
        this.setStatus('DISCONNECTED');
        this.scheduleReconnect();
      };

      this.socket.onerror = () => {
        this.setStatus('DISCONNECTED');
        if (this.socket) {
          this.socket.close();
        }
      };
    } catch (e) {
      this.setStatus('DISCONNECTED');
      this.scheduleReconnect();
    }
  }

  public disconnect() {
    this.autoReconnect = false;
    if (this.reconnectTimer) {
      clearTimeout(this.reconnectTimer);
      this.reconnectTimer = null;
    }
    if (this.socket) {
      this.socket.close();
      this.socket = null;
    }
    this.setStatus('DISCONNECTED');
  }

  public sendCommand(intersectionId: number, command: string, params: Record<string, any> = {}) {
    if (!this.socket || this.socket.readyState !== WebSocket.OPEN) {
      return false;
    }

    const payload = {
      timestamp: new Date().toISOString(),
      intersection: intersectionId,
      command,
      sequence: Date.now(),
      ...params,
    };

    this.socket.send(JSON.stringify(payload));
    return true;
  }

  public subscribe(listener: MessageListener): () => void {
    this.listeners.add(listener);
    return () => this.listeners.delete(listener);
  }

  public onStatusChange(listener: (status: WebSocketStatus) => void): () => void {
    this.statusListeners.add(listener);
    listener(this.status);
    return () => this.statusListeners.delete(listener);
  }

  private setStatus(status: WebSocketStatus) {
    this.status = status;
    this.statusListeners.forEach((l) => l(status));
  }

  private notifyListeners(data: any) {
    this.listeners.forEach((l) => l(data));
  }

  private scheduleReconnect() {
    if (!this.autoReconnect || this.reconnectTimer) return;
    this.reconnectTimer = window.setTimeout(() => {
      this.reconnectTimer = null;
      this.connect();
    }, 3000);
  }
}

export const wsService = new WebSocketService();
