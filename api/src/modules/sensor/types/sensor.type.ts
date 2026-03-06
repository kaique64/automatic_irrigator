export enum SensorTypeEnum {
  AIR = 'air',
  SOIL = 'soil',
}

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
  air: AirSensorMessage;
  soil: SoilSensorMessage;
}
