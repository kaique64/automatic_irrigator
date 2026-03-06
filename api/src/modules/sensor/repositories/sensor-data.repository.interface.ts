import { SensorData } from '../entities/sensor-data.entity';
import { SensorMessage } from '../types/sensor.type';

export interface SensorDataRepositoryInterface {
  save(sensorMessage: SensorMessage): Promise<SensorData>;
}
