import { AirSensorMessage } from '../types/air-sensor.type';

export interface AirSensorServiceInterface {
  dispatchSensorEvent(data: AirSensorMessage): void;
}
