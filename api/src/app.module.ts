import { Module } from '@nestjs/common';
import { ConfigModule } from '@nestjs/config';
import { SensorModule } from './modules/sensor/sensor.module';

@Module({
  imports: [
    ConfigModule.forRoot({
      isGlobal: true,
      envFilePath: '.env',
    }),
    SensorModule,
  ],
  controllers: [],
  providers: [],
})
export class AppModule {}
