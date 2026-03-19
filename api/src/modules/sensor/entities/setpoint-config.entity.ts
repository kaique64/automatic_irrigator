import { Column, Entity, PrimaryGeneratedColumn } from 'typeorm';

@Entity('setpoint_config')
export class SetpointConfig {
  @PrimaryGeneratedColumn('uuid')
  id: string;

  @Column({ type: 'float', name: 'setpoint' })
  setpoint: number;
}
