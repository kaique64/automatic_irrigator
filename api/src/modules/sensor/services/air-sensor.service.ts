import { Injectable, Logger } from '@nestjs/common';
import { AirSensorMessage } from '../types/air-sensor.type';
import { AirSensorServiceInterface } from './air-sensor.service.interface';
import { EventsGateway } from '../gateways/events.gateway';

@Injectable()
export class AirSensorService implements AirSensorServiceInterface {
  private AIR_SENSOR_EVENT_NAME = 'airSensor';

  private logger = new Logger(AirSensorService.name);

  constructor(private readonly eventsGateway: EventsGateway) {}

  dispatchSensorEvent(data: AirSensorMessage): void {
    this.logger.log(`Sending air sensor data: ${JSON.stringify(data)}`);

    this.eventsGateway.sendMessage<AirSensorMessage>(
      this.AIR_SENSOR_EVENT_NAME,
      data,
    );
  }
}
