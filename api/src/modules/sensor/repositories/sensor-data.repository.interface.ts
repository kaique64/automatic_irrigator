import { SensorData } from '../entities/sensor-data.entity';

export interface RawBucketRow {
  bucket: string;
  air_temperature: string | null;
  air_humidity: string | null;
  soil_humidity: string | null;
}

export interface SensorDataRepositoryInterface {
  save(sensorData: SensorData): Promise<SensorData>;
  findByTimeRange(hours: number): Promise<RawBucketRow[]>;
  findByDateRange(from: Date, to: Date): Promise<RawBucketRow[]>;
}
