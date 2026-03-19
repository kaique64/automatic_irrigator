import { forwardRef, Inject, Logger } from '@nestjs/common';
import {
  OnGatewayConnection,
  OnGatewayDisconnect,
  SubscribeMessage,
  WebSocketGateway,
  WebSocketServer,
} from '@nestjs/websockets';
import { Server, Socket } from 'socket.io';
import { SensorController } from '../controllers/sensor.controller';
import type { SetpoingConfigMessage } from '../types/setpoint-config.type';

@WebSocketGateway({
  cors: {
    origin: '*',
  },
})
export class EventsGateway implements OnGatewayConnection, OnGatewayDisconnect {
  private logger = new Logger(EventsGateway.name);

  @WebSocketServer()
  server: Server;

  constructor(
    @Inject(forwardRef(() => SensorController))
    private readonly sensorController: SensorController,
  ) {}

  handleConnection(client: Socket) {
    this.logger.log(`Client connected: ${client.id}`);
  }

  handleDisconnect(client: Socket) {
    this.logger.log(`Client disconnected: ${client.id}`);
  }

  sendMessage<T>(eventName: string, message: T) {
    this.server.emit(eventName, message);
  }

  @SubscribeMessage('setpoint-config')
  async handleSetpointConfigMessage(
    client: Socket,
    data: SetpoingConfigMessage,
  ) {
    await this.sensorController.handleSetpointConfig(data);
  }
}
