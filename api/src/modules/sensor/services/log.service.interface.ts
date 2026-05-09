import { Log } from '../entities/log.entity';
import { LogMessage } from '../types/log.type';

export interface LogServiceInterface {
  handleLog(data: LogMessage): Promise<void>;
  getRecentLogs(limit: number): Promise<Log[]>;
}
