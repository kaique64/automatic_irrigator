import { Log } from '../entities/log.entity';

export interface LogRepositoryInterface {
  save(log: Log): Promise<Log>;
  findRecent(limit: number): Promise<Log[]>;
}
