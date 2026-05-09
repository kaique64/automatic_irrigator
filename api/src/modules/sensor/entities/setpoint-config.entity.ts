import { Column, Entity, PrimaryGeneratedColumn } from 'typeorm';

@Entity('setpoint_config')
export class SetpointConfig {
  @PrimaryGeneratedColumn('uuid')
  id: string;

  @Column({ type: 'float', name: 'soil_setpoint' })
  soilSetpoint: number;

  @Column({ type: 'float', name: 'air_temperature_setpoint', nullable: true })
  airTemperatureSetpoint: number | null;
}
