import { Controller } from '@nestjs/common';
import { MessagePattern, Payload } from '@nestjs/microservices';
import { SensorService } from '../services/sensor.service';
import type { SensorMessage } from '../types/sensor.type';

@Controller()
export class SensorController {
  constructor(private readonly sensorService: SensorService) {}

  @MessagePattern('greenhouse/sensors')
  getMessage(@Payload() data: SensorMessage) {
    this.sensorService.dispatchSensorEvent(data);
  }
}
