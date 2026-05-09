import { Injectable } from '@nestjs/common';
import { DataSource } from 'typeorm';
import { Log } from '../entities/log.entity';
import { LogRepositoryInterface } from './log.repository.interface';

@Injectable()
export class LogRepository implements LogRepositoryInterface {
  constructor(private readonly dataSource: DataSource) {}

  async save(log: Log): Promise<Log> {
    return this.dataSource.getRepository(Log).save(log);
  }

  async findRecent(limit: number): Promise<Log[]> {
    return this.dataSource.getRepository(Log).find({
      order: { createdAt: 'DESC' },
      take: limit,
    });
  }
}
