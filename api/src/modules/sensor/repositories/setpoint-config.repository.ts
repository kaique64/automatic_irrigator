import { Injectable } from '@nestjs/common';
import { DataSource } from 'typeorm';
import { SetpointConfig } from '../entities/setpoint-config.entity';
import { SetpointConfigRepositoryInterface } from './setpoint-config.repository.interface';

@Injectable()
export class SetpointConfigRepository implements SetpointConfigRepositoryInterface {
  constructor(private readonly dataSource: DataSource) {}

  async save(setpointConfig: SetpointConfig): Promise<void> {
    await this.dataSource.getRepository(SetpointConfig).save(setpointConfig);
  }

  async getCurrentSetpoint(): Promise<SetpointConfig | null> {
    return await this.dataSource
      .getRepository(SetpointConfig)
      .createQueryBuilder('setpoint')
      .orderBy('setpoint.id', 'DESC')
      .limit(1)
      .getOne();
  }
}
