import { Sensor } from './sensor.type';

export interface AirSensorMessage extends Sensor {
  currentHumidity: number;
  currentTemperature: number;
}
