import { Injectable } from '@nestjs/common';
import { DataSource } from 'typeorm';
import { SensorData } from '../entities/sensor-data.entity';
import { RawBucketRow, SensorDataRepositoryInterface } from './sensor-data.repository.interface';

const BUCKET_INTERVAL_MAP: Record<number, string> = {
  1:  '1 minute',
  2:  '2 minutes',
  4:  '4 minutes',
  8:  '8 minutes',
  12: '12 minutes',
  24: '24 minutes',
  48: '48 minutes',
  72: '72 minutes',
};

@Injectable()
export class SensorDataRepository implements SensorDataRepositoryInterface {
  constructor(private readonly dataSource: DataSource) {}

  async save(sensorData: SensorData): Promise<SensorData> {
    return this.dataSource.getRepository(SensorData).save(sensorData);
  }

  async findByTimeRange(hours: number): Promise<RawBucketRow[]> {
    const bucket = BUCKET_INTERVAL_MAP[hours];
    if (!bucket) throw new Error(`Unsupported hours value: ${hours}`);
    return this.dataSource.query(
      `SELECT
        time_bucket($1::interval, timestamp) AS bucket,
        AVG((raw_data->'air'->>'temperature')::numeric) AS air_temperature,
        AVG((raw_data->'air'->>'humidity')::numeric)    AS air_humidity,
        AVG((raw_data->'soil'->>'humidity')::numeric)   AS soil_humidity
       FROM sensor_data
       WHERE timestamp >= NOW() - $2::interval
       GROUP BY bucket
       ORDER BY bucket ASC`,
      [bucket, `${hours} hours`],
    );
  }
}
