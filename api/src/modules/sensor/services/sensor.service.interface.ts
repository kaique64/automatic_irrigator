import { HistoricalDataResponse, SensorMessage } from '../types/sensor.type';

export interface SensorServiceInterface {
  handleSensorData(data: SensorMessage): Promise<void>;
  getHistoricalData(hours: number): Promise<HistoricalDataResponse>;
}
