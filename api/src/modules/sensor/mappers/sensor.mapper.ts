import { Injectable } from '@nestjs/common';
import { SensorData } from '../entities/sensor-data.entity';
import { SensorMessage } from '../types/sensor.type';
import { SensorDataBuilder } from '../builders/sensor-data.builder';

@Injectable()
export class SensorMapper {
  static toEntity({
    deviceId,
    timestamp,
    sensor_data_moment_id,
    air,
    soil,
  }: SensorMessage): SensorData {
    const airScore  = (air?.temperature ?? 0) + (air?.humidity ?? 0);
    const soilScore = soil?.humidity ?? 0;
    const airWins   = airScore >= soilScore;

    return SensorDataBuilder.create()
      .withDeviceId(deviceId)
      .withTimestamp(new Date())
      .withSensorType(airWins ? 'air' : 'soil')
      .withSensorDataMomentId(sensor_data_moment_id ?? null)
      .withTemperature(airWins ? (air?.temperature ?? null) : null)
      .withHumidity(airWins ? (air?.humidity ?? 0) : (soil?.humidity ?? 0))
      .withRawData({ deviceId, timestamp, sensor_data_moment_id, air, soil })
      .build();
  }
}
