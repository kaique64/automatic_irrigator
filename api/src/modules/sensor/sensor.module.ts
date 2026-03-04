import { Module } from '@nestjs/common';
import { SensorController } from './controllers/sensor.controller';
import { AirSensorService } from './services/air-sensor.service';
import { EventsGateway } from './gateways/events.gateway';

@Module({
  controllers: [SensorController],
  providers: [AirSensorService, EventsGateway],
  exports: [EventsGateway],
  imports: [],
})
export class SensorModule {}
