import { BadRequestException, Controller, Get, Query, ParseIntPipe } from '@nestjs/common';
import { MessagePattern, Payload } from '@nestjs/microservices';
import { SensorService } from '../services/sensor.service';
import { SetpointConfigService } from '../services/setpoint-config.service';
import type { SensorMessage, HistoricalDataResponse } from '../types/sensor.type';
import type { SetpoingConfigMessage } from '../types/setpoint-config.type';
import { SubscribeMessage } from '@nestjs/websockets';

const ALLOWED_HOURS = [1, 2, 4, 8, 12, 24, 48, 72];

@Controller()
export class SensorController {
  constructor(
    private readonly sensorService: SensorService,
    private readonly setpointConfigService: SetpointConfigService,
  ) {}

  @MessagePattern('greenhouse/sensors', { qos: 2 })
  async getSensorData(@Payload() data: SensorMessage) {
    await this.sensorService.handleSensorData(data);
  }

  @SubscribeMessage('setpoint-config')
  async handleSetpointConfig(@Payload() data: SetpoingConfigMessage) {
    await this.setpointConfigService.updateSetpointConfig(data);
  }

  @Get('setpoint-config')
  async getSetpointConfig() {
    const setpoint = await this.setpointConfigService.getSetpointConfig();
    return setpoint ?? { setpoint: 70 };
  }

  @Get('sensor-data/history')
  async getHistory(
    @Query('hours', ParseIntPipe) hours: number,
  ): Promise<HistoricalDataResponse> {
    if (!ALLOWED_HOURS.includes(hours)) {
      throw new BadRequestException(
        `hours must be one of: ${ALLOWED_HOURS.join(', ')}`,
      );
    }
    return this.sensorService.getHistoricalData(hours);
  }

  @Get('sensor-data/history/range')
  async getHistoryByRange(
    @Query('from') from: string,
    @Query('to') to: string,
  ): Promise<HistoricalDataResponse> {
    const fromDate = new Date(from);
    const toDate   = new Date(to);

    if (isNaN(fromDate.getTime()) || isNaN(toDate.getTime())) {
      throw new BadRequestException('from and to must be valid ISO date strings');
    }
    if (fromDate >= toDate) {
      throw new BadRequestException('from must be before to');
    }

    return this.sensorService.getHistoricalDataByRange(fromDate, toDate);
  }
}
