/* eslint-disable @typescript-eslint/no-unsafe-assignment */
import { Injectable } from '@nestjs/common';
import { SensorData } from '../entities/sensor-data.entity';
import { SensorMessage, SensorTypeEnum } from '../types/sensor.type';

@Injectable()
export class SensorMapper {
  static toEntity({
    deviceId,
    timestamp,
    air,
    soil,
  }: SensorMessage): SensorData {
    const sensorData = new SensorData();
    sensorData.deviceId = deviceId;
    sensorData.timestamp = new Date(timestamp);
    sensorData.rawData = { deviceId, timestamp, air, soil };

    sensorData.sensorType = SensorTypeEnum.AIR;
    sensorData.temperature = air.temperature;
    sensorData.humidity = air.humidity;
    sensorData.sensorType = SensorTypeEnum.SOIL;
    sensorData.humidity = soil.humidity;
    sensorData.temperature = null;

    return sensorData;
  }
}
