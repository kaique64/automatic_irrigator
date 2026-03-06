import { Injectable } from '@nestjs/common';
import { InjectRepository } from '@nestjs/typeorm';
import { Repository } from 'typeorm';
import { SensorData } from '../entities/sensor-data.entity';
import { SensorMessage } from '../types/sensor.type';
import { SensorMapper } from '../mappers/sensor.mapper';
import { SensorDataRepositoryInterface } from './sensor-data.repository.interface';

@Injectable()
export class SensorDataRepository implements SensorDataRepositoryInterface {
  constructor(
    @InjectRepository(SensorData)
    private readonly repository: Repository<SensorData>,
  ) {}

  async save(sensorMessage: SensorMessage): Promise<SensorData> {
    const sensorData = SensorMapper.toEntity(sensorMessage);

    return await this.repository.save(sensorData);
  }
}
