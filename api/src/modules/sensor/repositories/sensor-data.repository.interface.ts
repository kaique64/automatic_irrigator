import { SensorData } from '../entities/sensor-data.entity';

export interface SensorDataRepositoryInterface {
  save(sensorData: SensorData[]): Promise<SensorData[]>;
}
