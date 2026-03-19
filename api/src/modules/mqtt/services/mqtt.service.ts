import {
  Injectable,
  Logger,
  OnModuleInit,
  OnModuleDestroy,
} from '@nestjs/common';
import * as mqtt from 'mqtt';

@Injectable()
export class MqttService implements OnModuleInit, OnModuleDestroy {
  private readonly logger = new Logger(MqttService.name);
  private client: mqtt.MqttClient;
  private isConnected = false;

  async onModuleInit() {
    await this.connect();
  }

  private async connect(): Promise<void> {
    return new Promise((resolve, reject) => {
      const options: mqtt.IClientOptions = {
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
        clientId: `nest-mqtt-client-${Math.random().toString(16).slice(2, 10)}`,
      };

      const brokerUrl = process.env.MQTT_BROKER_URL || 'mqtt://localhost:1883';
      this.client = mqtt.connect(brokerUrl, options);

      this.client.on('connect', () => {
        this.isConnected = true;
        this.logger.log('Connected to MQTT broker');
        resolve();
      });

      this.client.on('error', (error) => {
        this.logger.error('MQTT connection error:', error);
        reject(error);
      });

      this.client.on('reconnect', () => {
        this.logger.log('Reconnecting to MQTT broker...');
      });

      this.client.on('offline', () => {
        this.isConnected = false;
        this.logger.warn('MQTT client is offline');
      });

      this.client.on('disconnect', () => {
        this.isConnected = false;
        this.logger.log('Disconnected from MQTT broker');
      });
    });
  }

  async publish(topic: string, message: string | object): Promise<void> {
    return new Promise((resolve, reject) => {
      if (!this.isConnected) {
        const error = new Error('MQTT client is not connected');
        this.logger.error(`Failed to publish to ${topic}: ${error.message}`);
        return reject(error);
      }

      const payload =
        typeof message === 'string' ? message : JSON.stringify(message);

      this.logger.log(`Publishing to ${topic}: ${payload}`);

      this.client.publish(topic, payload, { qos: 1 }, (error) => {
        if (error) {
          this.logger.error(`Failed to publish to ${topic}:`, error);
          reject(error);
        } else {
          this.logger.log(`Successfully published to ${topic}`);
          resolve();
        }
      });
    });
  }

  async subscribe(
    topic: string,
    callback: (message: string) => void,
  ): Promise<void> {
    return new Promise((resolve, reject) => {
      if (!this.isConnected) {
        const error = new Error('MQTT client is not connected');
        this.logger.error(`Failed to subscribe to ${topic}: ${error.message}`);
        return reject(error);
      }

      this.client.subscribe(topic, { qos: 1 }, (error) => {
        if (error) {
          this.logger.error(`Failed to subscribe to ${topic}:`, error);
          reject(error);
        } else {
          this.logger.log(`Subscribed to ${topic}`);
          resolve();
        }
      });

      this.client.on('message', (receivedTopic, payload) => {
        if (receivedTopic === topic) {
          callback(payload.toString());
        }
      });
    });
  }

  async onModuleDestroy() {
    if (this.client) {
      this.logger.log('Closing MQTT client');
      await new Promise<void>((resolve) => {
        this.client.end(false, {}, () => {
          this.logger.log('MQTT client closed');
          resolve();
        });
      });
    }
  }
}
