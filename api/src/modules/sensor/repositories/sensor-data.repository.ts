import { Injectable } from '@nestjs/common';
import { DataSource } from 'typeorm';
import { SensorData } from '../entities/sensor-data.entity';
import { SensorMapper } from '../mappers/sensor.mapper';
import { SensorMessage } from '../types/sensor.type';
import { SensorDataRepositoryInterface } from './sensor-data.repository.interface';

@Injectable()
export class SensorDataRepository implements SensorDataRepositoryInterface {
  constructor(private readonly dataSource: DataSource) {}

  async save(sensorMessage: SensorMessage): Promise<SensorData[]> {
    const sensorData = SensorMapper.toEntities(sensorMessage);

    return await this.dataSource.transaction(async (manager) => {
      return await manager.save(SensorData, sensorData);
    });
  }
}
