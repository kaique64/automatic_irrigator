import { Inject, Injectable, Logger } from '@nestjs/common';
import { ClientProxy } from '@nestjs/microservices';
import { lastValueFrom } from 'rxjs';

@Injectable()
export class MqttService {
  private readonly logger = new Logger(MqttService.name);

  constructor(@Inject('MQTT_CLIENT') private readonly client: ClientProxy) {}

  async publish(topic: string, message: string | object): Promise<void> {
    try {
      const payload = typeof message === 'string' ? message : message;

      this.logger.log(`Publishing to ${topic}: ${JSON.stringify(payload)}`);

      await lastValueFrom(this.client.emit(topic, payload));

      this.logger.log(`Successfully published to ${topic}`);
    } catch (error) {
      this.logger.error(`Failed to publish to ${topic}:`, error);
      throw error;
    }
  }

  async onModuleDestroy() {
    if (this.client) {
      this.logger.log('Closing MQTT client');
      await this.client.close();
    }
  }
}
