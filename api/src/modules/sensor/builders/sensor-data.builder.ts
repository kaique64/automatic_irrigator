import { SensorData } from '../entities/sensor-data.entity';
import { SensorMessage } from '../types/sensor.type';

export class SensorDataBuilder {
  private sensorData: SensorData;

  constructor() {
    this.sensorData = new SensorData();
  }

  withDeviceId(deviceId: string): this {
    this.sensorData.deviceId = deviceId;
    return this;
  }

  withTimestamp(timestamp: Date | string | number): this {
    if (timestamp instanceof Date) {
      this.sensorData.timestamp = timestamp;
    } else {
      this.sensorData.timestamp = new Date(timestamp);
    }
    return this;
  }

  withSensorType(sensorType: string): this {
    this.sensorData.sensorType = sensorType;
    return this;
  }

  withTemperature(temperature: number | null): this {
    this.sensorData.temperature = temperature;
    return this;
  }

  withHumidity(humidity: number): this {
    this.sensorData.humidity = humidity;
    return this;
  }

  withRawData(rawData: SensorMessage): this {
    this.sensorData.rawData = rawData;
    return this;
  }

  withId(id: string): this {
    this.sensorData.id = id;
    return this;
  }

  withCreatedAt(createdAt: Date): this {
    this.sensorData.createdAt = createdAt;
    return this;
  }

  build(): SensorData {
    return this.sensorData;
  }

  static create(): SensorDataBuilder {
    return new SensorDataBuilder();
  }
}
