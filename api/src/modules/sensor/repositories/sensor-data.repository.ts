import { Injectable } from '@nestjs/common';
import { DataSource } from 'typeorm';
import { SensorData } from '../entities/sensor-data.entity';
import { SensorDataRepositoryInterface } from './sensor-data.repository.interface';

@Injectable()
export class SensorDataRepository implements SensorDataRepositoryInterface {
  constructor(private readonly dataSource: DataSource) {}

  async save(sensorData: SensorData[]): Promise<SensorData[]> {
    return await this.dataSource.transaction(async (manager) => {
      return await manager.save(SensorData, sensorData);
    });
  }
}
