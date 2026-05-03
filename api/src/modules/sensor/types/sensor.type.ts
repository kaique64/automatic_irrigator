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
  sensor_data_moment_id: string;
  air: AirSensorMessage;
  soil: SoilSensorMessage;
}

export interface HistoricalDataPoint {
  timestamp: Date;
  air: {
    temperature: number | null;
    humidity: number | null;
  };
  soil: {
    humidity: number | null;
  };
}

export interface HistoricalDataResponse {
  points: HistoricalDataPoint[];
  average: {
    air: { temperature: number | null; humidity: number | null };
    soil: { humidity: number | null };
  };
}
