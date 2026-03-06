import { Entity, Column, PrimaryGeneratedColumn, Index } from 'typeorm';

@Entity('sensor_data')
@Index(['deviceId', 'timestamp'])
@Index(['timestamp'])
export class SensorData {
  @PrimaryGeneratedColumn('uuid')
  id: string;

  @Column({ type: 'varchar', length: 255, name: 'device_id' })
  deviceId: string;

  @Column({ type: 'timestamptz', name: 'timestamp' })
  timestamp: Date;

  @Column({ type: 'varchar', length: 50, name: 'sensor_type' })
  sensorType: string;

  @Column({ type: 'decimal', precision: 5, scale: 2, nullable: true })
  temperature: number | null;

  @Column({ type: 'decimal', precision: 5, scale: 2, nullable: true })
  humidity: number;

  @Column({ type: 'jsonb', nullable: true, name: 'raw_data' })
  rawData: any;

  @Column({
    type: 'timestamptz',
    name: 'created_at',
    default: () => 'CURRENT_TIMESTAMP',
  })
  createdAt: Date;
}
