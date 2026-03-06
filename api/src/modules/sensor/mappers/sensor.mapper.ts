import { Injectable } from '@nestjs/common';
import { SensorData } from '../entities/sensor-data.entity';
import { SensorMessage, SensorTypeEnum } from '../types/sensor.type';
import { SensorDataBuilder } from '../builders/sensor-data.builder';

@Injectable()
export class SensorMapper {
  static toEntities({
    deviceId,
    timestamp,
    air,
    soil,
  }: SensorMessage): SensorData[] {
    const rawData = { deviceId, timestamp, air, soil };
    const parsedTimestamp = new Date(timestamp);

    const airSensorData = SensorDataBuilder.create()
      .withDeviceId(deviceId)
      .withTimestamp(parsedTimestamp)
      .withSensorType(SensorTypeEnum.AIR)
      .withTemperature(air.temperature)
      .withHumidity(air.humidity)
      .withRawData(rawData)
      .build();

    const soilSensorData = SensorDataBuilder.create()
      .withDeviceId(deviceId)
      .withTimestamp(parsedTimestamp)
      .withSensorType(SensorTypeEnum.SOIL)
      .withTemperature(null)
      .withHumidity(soil.humidity)
      .withRawData(rawData)
      .build();

    return [airSensorData, soilSensorData];
  }
}
