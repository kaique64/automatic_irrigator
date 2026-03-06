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
  extra: {
    max: configService.get<number>('POSTGRES_DB_POOL_MAX', 10),
    min: configService.get<number>('POSTGRES_DB_POOL_MIN', 2),
    idleTimeoutMillis: configService.get<number>(
      'POSTGRES_DB_IDLE_TIMEOUT',
      30000,
    ),
    connectionTimeoutMillis: configService.get<number>(
      'POSTGRES_DB_CONNECTION_TIMEOUT',
      10000,
    ),
  },
  retryAttempts: configService.get<number>('POSTGRES_DB_RETRY_ATTEMPTS', 10),
  retryDelay: configService.get<number>('POSTGRES_DB_RETRY_DELAY', 10000),
  autoLoadEntities: true,
  connectTimeoutMS: configService.get<number>(
    'POSTGRES_DB_CONNECTION_TIMEOUT',
    10000,
  ),
});
