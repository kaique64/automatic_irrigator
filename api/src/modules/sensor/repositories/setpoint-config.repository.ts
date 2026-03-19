import { Injectable } from '@nestjs/common';
import { DataSource } from 'typeorm';
import { SetpointConfig } from '../entities/setpoint-config.entity';
import { SetpointConfigRepositoryInterface } from './setpoint-config.repository.interface';

@Injectable()
export class SetpointConfigRepository implements SetpointConfigRepositoryInterface {
  constructor(private readonly dataSource: DataSource) {}

  async save(setpointConfig: SetpointConfig): Promise<void> {
    await this.dataSource.transaction(async (manager) => {
      const existingSetpoint = await manager
        .getRepository(SetpointConfig)
        .createQueryBuilder('setpoint')
        .orderBy('setpoint.id', 'DESC')
        .limit(1)
        .getOne();

      if (existingSetpoint) {
        existingSetpoint.setpoint = setpointConfig.setpoint;
        await manager.save(SetpointConfig, existingSetpoint);
      } else {
        await manager.save(SetpointConfig, setpointConfig);
      }
    });
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
