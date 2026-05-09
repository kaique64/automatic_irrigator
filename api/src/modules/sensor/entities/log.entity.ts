import { Entity, Column, PrimaryGeneratedColumn, Index } from 'typeorm';

@Entity('logs')
@Index(['createdAt'])
export class Log {
  @PrimaryGeneratedColumn('uuid')
  id: string;

  @Column({ type: 'timestamptz' })
  timestamp: Date;

  @Column({ type: 'varchar', length: 50, nullable: true })
  level: string | null;

  @Column({ type: 'text' })
  message: string;

  @Column({ type: 'varchar', length: 255, nullable: true, name: 'device_id' })
  deviceId: string | null;

  @Column({ type: 'jsonb', nullable: true, name: 'raw_data' })
  rawData: any;

  @Column({
    type: 'timestamptz',
    name: 'created_at',
    default: () => 'CURRENT_TIMESTAMP',
  })
  createdAt: Date;
}
