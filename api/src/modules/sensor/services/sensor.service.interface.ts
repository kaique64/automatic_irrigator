import { SensorMessage } from '../types/sensor.type';

export interface SensorServiceInterface {
  dispatchSensorEvent(data: SensorMessage): Promise<void>;
}
