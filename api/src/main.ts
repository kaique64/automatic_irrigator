import { NestFactory } from '@nestjs/core';
import { AppModule } from './app.module';
import { MicroserviceOptions, Transport } from '@nestjs/microservices';
import { Logger } from '@nestjs/common';

async function bootstrap() {
  const logger = new Logger('Bootstrap');

  const app = await NestFactory.create(AppModule);

  app.connectMicroservice<MicroserviceOptions>({
    transport: Transport.MQTT,
    options: {
      url: process.env.MQTT_BROKER_URL,
      username: process.env.MQTT_BROKER_USERNAME,
      password: process.env.MQTT_BROKER_PASSWORD,
      reconnectPeriod: process.env.MQTT_BROKER_RECONNECT_PERIOD
        ? parseInt(process.env.MQTT_BROKER_RECONNECT_PERIOD)
        : 5000,
      connectTimeout: process.env.MQTT_BROKER_CONNECT_TIMEOUT
        ? parseInt(process.env.MQTT_BROKER_CONNECT_TIMEOUT)
        : 30000,
      keepalive: process.env.MQTT_BROKER_KEEP_ALIVE
        ? parseInt(process.env.MQTT_BROKER_KEEP_ALIVE)
        : 60,
      clean: true,
      clientId: `automatic-irrigator-${Math.random().toString(16).slice(2, 10)}`,
      resubscribe: true,
    },
  });

  await app.startAllMicroservices();

  logger.log('MQTT Connected Successfully!');

  await app.listen(process.env.PORT ?? 3000);

  logger.log(`HTTP Server running on port ${process.env.PORT ?? 3000}`);
}

bootstrap().catch((error) => {
  const logger = new Logger('Bootstrap');
  logger.error('Failed to start application', error);
  process.exit(1);
});
