import { Entity, Column, PrimaryGeneratedColumn, Index } from 'typeorm';
import type { SensorMessage } from '../types/sensor.type';
import { Hypertable, TimeColumn } from '@timescaledb/typeorm';

@Hypertable({})
@Entity('sensor_data')
@Index(['deviceId', 'timestamp'])
@Index(['timestamp'])
export class SensorData {
  @PrimaryGeneratedColumn('uuid')
  id: string;

  @Column({ type: 'varchar', length: 255, name: 'device_id' })
  deviceId: string;

  @TimeColumn()
  timestamp: Date;

  @Column({ type: 'varchar', length: 50, name: 'sensor_type' })
  sensorType: string;

  @Column({ type: 'decimal', precision: 5, scale: 2, nullable: true })
  temperature: number | null;

  @Column({ type: 'decimal', precision: 5, scale: 2, nullable: true })
  humidity: number;

  @Column({ type: 'jsonb', nullable: true, name: 'raw_data' })
  rawData: SensorMessage;

  @Column({
    type: 'timestamptz',
    name: 'created_at',
    default: () => 'CURRENT_TIMESTAMP',
  })
  createdAt: Date;
}
