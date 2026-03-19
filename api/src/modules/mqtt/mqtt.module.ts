import { Module, DynamicModule } from '@nestjs/common';
import { ClientsModule, Transport } from '@nestjs/microservices';
import { MqttService } from './services/mqtt.service';

@Module({})
export class MqttModule {
  static forRoot(): DynamicModule {
    return {
      module: MqttModule,
      imports: [
        ClientsModule.register([
          {
            name: 'MQTT_CLIENT',
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
              clientId: `mqtt-client-${Math.random().toString(16).slice(2, 10)}`,
              resubscribe: true,
            },
          },
        ]),
      ],
      providers: [MqttService],
      exports: [MqttService],
    };
  }
}
