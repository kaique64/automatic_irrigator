import { Injectable, Logger } from '@nestjs/common';
import { SensorMessage } from '../types/sensor.type';
import { SensorServiceInterface } from './sensor.service.interface';
import { EventsGateway } from '../gateways/events.gateway';

@Injectable()
export class SensorService implements SensorServiceInterface {
  private SENSOR_EVENT_NAME = 'sensors';

  private logger = new Logger(SensorService.name);

  constructor(private readonly eventsGateway: EventsGateway) {}

  dispatchSensorEvent(data: SensorMessage): void {
    this.logger.log(`Sending air sensor data: ${JSON.stringify(data)}`);

    this.eventsGateway.sendMessage<SensorMessage>(this.SENSOR_EVENT_NAME, data);
  }
}
