import { Module } from '@nestjs/common';
import { SensorController } from './controllers/sensor.controller';
import { SensorService } from './services/sensor.service';
import { EventsGateway } from './gateways/events.gateway';

@Module({
  controllers: [SensorController],
  providers: [SensorService, EventsGateway],
  exports: [EventsGateway],
  imports: [],
})
export class SensorModule {}
