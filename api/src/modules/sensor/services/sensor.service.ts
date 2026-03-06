import { Injectable, Logger } from '@nestjs/common';
import { SensorMessage } from '../types/sensor.type';
import { SensorServiceInterface } from './sensor.service.interface';
import { EventsGateway } from '../gateways/events.gateway';
import { SensorDataRepository } from '../repositories/sensor-data.repository';

@Injectable()
export class SensorService implements SensorServiceInterface {
  private SENSOR_EVENT_NAME = 'sensors';

  private logger = new Logger(SensorService.name);

  constructor(
    private readonly sensorDataRepository: SensorDataRepository,
    private readonly eventsGateway: EventsGateway,
  ) {}

  async dispatchSensorEvent(data: SensorMessage): Promise<void> {
    try {
      this.logger.log(`Saving sensor data: ${JSON.stringify(data)}`);

      await this.sensorDataRepository.save(data);

      this.logger.log('Sensor data saved successfully');
    } catch (err) {
      this.logger.error('Error saving sensor data', err);
      throw err;
    }

    this.logger.log('Sending sensor data');

    this.eventsGateway.sendMessage<SensorMessage>(this.SENSOR_EVENT_NAME, data);

    this.logger.log('Sensor data sent successfully');
  }
}
