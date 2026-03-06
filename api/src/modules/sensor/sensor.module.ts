import { Module } from '@nestjs/common';
import { SensorController } from './controllers/sensor.controller';
import { SensorService } from './services/sensor.service';
import { EventsGateway } from './gateways/events.gateway';
import { SensorDataRepository } from './repositories/sensor-data.repository';
import { TypeOrmModule } from '@nestjs/typeorm';
import { SensorData } from './entities/sensor-data.entity';

@Module({
  imports: [TypeOrmModule.forFeature([SensorData])],
  controllers: [SensorController],
  providers: [SensorService, SensorDataRepository, EventsGateway],
  exports: [EventsGateway],
})
export class SensorModule {}
