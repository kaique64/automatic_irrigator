import { forwardRef, Inject, Injectable, Logger } from '@nestjs/common';
import { SetpointConfigServiceInterface } from './setpoint-config.service.interface';
import { SetpoingConfigMessage } from '../types/setpoint-config.type';
import { MqttService } from 'src/modules/mqtt/services/mqtt.service';
import { SetpointConfigRepository } from '../repositories/setpoint-config.repository';
import { SetpointConfigMapper } from '../mappers/setpoint-config.mapper';
import { EventsGateway } from '../gateways/events.gateway';

@Injectable()
export class SetpointConfigService implements SetpointConfigServiceInterface {
  private readonly SETPOINT_QUEUE = 'greenhouse/setpoint';
  private readonly SETPOINT_CONFIG_EVENT_NAME = 'setpoint-config';

  private logger = new Logger(SetpointConfigService.name);

  constructor(
    private readonly setpointConfigRepository: SetpointConfigRepository,
    @Inject(forwardRef(() => EventsGateway))
    private readonly eventsGateway: EventsGateway,
    private readonly mqttService: MqttService,
  ) {}

  async updateSetpointConfig(
    setpointConfigMessage: SetpoingConfigMessage,
  ): Promise<void> {
    try {
      const setpoint = SetpointConfigMapper.toEntity(setpointConfigMessage);

      await this.setpointConfigRepository.save(setpoint);

      await this.mqttService.publish(this.SETPOINT_QUEUE, setpoint);
    } catch (err) {
      this.logger.error('Error updating setpoing config', err);

      this.eventsGateway.sendMessage(this.SETPOINT_CONFIG_EVENT_NAME, {
        error: (err as Error).message,
      });

      throw err;
    }

    this.eventsGateway.sendMessage(this.SETPOINT_CONFIG_EVENT_NAME, {
      message: `Setpoint config updated successfully to ${setpointConfigMessage.setpoint}`,
      success: true,
    });

    this.logger.log(
      `Setpoint config updated successfully to ${setpointConfigMessage.setpoint}`,
    );
  }
}
