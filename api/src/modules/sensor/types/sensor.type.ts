export interface AirSensorMessage {
  temperature: number;
  humidity: number;
}

export interface SoilSensorMessage {
  humidity: number;
}

export interface SensorMessage {
  deviceId: string;
  timestamp: string;
  data: AirSensorMessage | SoilSensorMessage;
}
