export interface LogMessage {
  tag: string;
  level?: string; // INFO, WARN, ERROR, DEBUG
  message: string;
  timestamp?: number; // ESP32 millis() — ms since boot, not a real clock
}
