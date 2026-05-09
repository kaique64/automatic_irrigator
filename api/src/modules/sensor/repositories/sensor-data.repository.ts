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

  async existsByMomentId(momentId: string): Promise<boolean> {
    const rows = await this.dataSource.query(
      `SELECT 1 FROM sensor_data WHERE sensor_data_moment_id = $1 LIMIT 1`,
      [momentId],
    );
    return rows.length > 0;
  }

  async findByDateRange(from: Date, to: Date): Promise<RawBucketRow[]> {
    const diffHours = (to.getTime() - from.getTime()) / 3_600_000;
    let bucket: string;
    if (diffHours <= 2)  bucket = '1 minute';
    else if (diffHours <= 12) bucket = '5 minutes';
    else if (diffHours <= 72) bucket = '30 minutes';
    else                      bucket = '2 hours';

    return this.dataSource.query(
      `SELECT
        time_bucket($1::interval, timestamp) AS bucket,
        AVG((raw_data->'air'->>'temperature')::numeric) AS air_temperature,
        AVG((raw_data->'air'->>'humidity')::numeric)    AS air_humidity,
        AVG((raw_data->'soil'->>'humidity')::numeric)   AS soil_humidity
       FROM sensor_data
       WHERE timestamp >= $2 AND timestamp <= $3
       GROUP BY bucket
       ORDER BY bucket ASC`,
      [bucket, from, to],
    );
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
