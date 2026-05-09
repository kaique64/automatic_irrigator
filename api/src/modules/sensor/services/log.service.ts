import { forwardRef, Inject, Injectable, Logger } from '@nestjs/common';
import { LogServiceInterface } from './log.service.interface';
import { LogMessage } from '../types/log.type';
import { Log } from '../entities/log.entity';
import { LogRepository } from '../repositories/log.repository';
import { EventsGateway } from '../gateways/events.gateway';

@Injectable()
export class LogService implements LogServiceInterface {
  private readonly LOG_EVENT_NAME = 'sensor-logs';
  private readonly logger = new Logger(LogService.name);

  constructor(
    private readonly logRepository: LogRepository,
    @Inject(forwardRef(() => EventsGateway))
    private readonly eventsGateway: EventsGateway,
  ) {}

  async handleLog(data: LogMessage): Promise<void> {
    const log = new Log();
    log.timestamp = new Date(); // ESP32 timestamp is millis() since boot — use server time
    log.level     = data.tag;
    log.message   = data.message;
    log.deviceId  = null;
    log.rawData   = data;

    await this.logRepository.save(log);

    this.eventsGateway.sendMessage(this.LOG_EVENT_NAME, log);
    this.logger.log(`[${log.level}] ${log.message}`);
  }

  async getRecentLogs(limit: number): Promise<Log[]> {
    return this.logRepository.findRecent(limit);
  }
}
