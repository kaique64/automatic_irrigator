import { Controller } from '@nestjs/common';
import { MessagePattern, Payload } from '@nestjs/microservices';
import { SensorService } from '../services/sensor.service';
import { SetpointConfigService } from '../services/setpoint-config.service';
import type { SensorMessage } from '../types/sensor.type';
import type { SetpoingConfigMessage } from '../types/setpoint-config.type';
import { SubscribeMessage } from '@nestjs/websockets';

@Controller()
export class SensorController {
  constructor(
    private readonly sensorService: SensorService,
    private readonly setpointConfigService: SetpointConfigService,
  ) {}

  @MessagePattern('greenhouse/sensors')
  async getSensorData(@Payload() data: SensorMessage) {
    await this.sensorService.handleSensorData(data);
  }

  @SubscribeMessage('setpoint-config')
  async handleSetpointConfig(@Payload() data: SetpoingConfigMessage) {
    await this.setpointConfigService.updateSetpointConfig(data);
  }
}
