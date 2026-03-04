import { Controller } from '@nestjs/common';
import { MessagePattern, Payload } from '@nestjs/microservices';
import { AirSensorService } from '../services/air-sensor.service';
import type { AirSensorMessage } from '../types/air-sensor.type';

@Controller()
export class SensorController {
  constructor(private readonly airSensorService: AirSensorService) {}

  @MessagePattern('greenhouse/air/temperature/current')
  getMessage(@Payload() data: AirSensorMessage) {
    this.airSensorService.dispatchSensorEvent(data);
  }
}
