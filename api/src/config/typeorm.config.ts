import { TypeOrmModuleOptions } from '@nestjs/typeorm';
import { ConfigService } from '@nestjs/config';
import { SensorData } from '../modules/sensor/entities/sensor-data.entity';

export const typeOrmConfigFactory = (
  configService: ConfigService,
): TypeOrmModuleOptions => ({
  type: 'postgres',
  host: configService.get<string>('POSTGRES_HOST', 'localhost'),
  port: configService.get<number>('POSTGRES_PORT', 5432),
  username: configService.get<string>('POSTGRES_USER', 'postgres'),
  password: configService.get<string>('POSTGRES_PASSWORD', 'postgres'),
  database: configService.get<string>('POSTGRES_DB', 'irrigator'),
  entities: [SensorData],
  synchronize: configService.get<boolean>('TYPEORM_SYNCHRONIZE', true),
  logging: configService.get<boolean>('TYPEORM_LOGGING', false),
});
