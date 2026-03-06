import { Controller } from '@nestjs/common';
import { MessagePattern, Payload } from '@nestjs/microservices';
import { SensorService } from '../services/sensor.service';
import type { SensorMessage } from '../types/sensor.type';

@Controller()
export class SensorController {
  constructor(private readonly sensorService: SensorService) {}

  @MessagePattern('greenhouse/sensors')
  async getSensorData(@Payload() data: SensorMessage) {
    await this.sensorService.handleSensorData(data);
  }
}
