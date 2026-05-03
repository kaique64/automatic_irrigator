import { Module } from '@nestjs/common';
import { TypeOrmModule } from '@nestjs/typeorm';
import { MqttModule } from '../mqtt/mqtt.module';
import { SensorController } from './controllers/sensor.controller';
import { SensorData } from './entities/sensor-data.entity';
import { EventsGateway } from './gateways/events.gateway';
import { SensorDataRepository } from './repositories/sensor-data.repository';
import { SensorService } from './services/sensor.service';
import { SetpointConfigService } from './services/setpoint-config.service';
import { SetpointConfigRepository } from './repositories/setpoint-config.repository';
import { SetpointConfig } from './entities/setpoint-config.entity';

@Module({
  imports: [
    MqttModule.forRoot(),
    TypeOrmModule.forFeature([SensorData, SetpointConfig]),
  ],
  controllers: [SensorController],
  providers: [
    SensorController,
    SensorService,
    SensorDataRepository,
    SetpointConfigService,
    SetpointConfigRepository,
    EventsGateway,
  ],
  exports: [EventsGateway],
})
export class SensorModule {}
