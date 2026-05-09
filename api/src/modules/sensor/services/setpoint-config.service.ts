import { BadRequestException, forwardRef, Inject, Injectable, Logger } from '@nestjs/common';
import { SetpointConfigServiceInterface } from './setpoint-config.service.interface';
import { SetpoingConfigMessage } from '../types/setpoint-config.type';
import { MqttService } from '../../mqtt/services/mqtt.service';
import { SetpointConfigRepository } from '../repositories/setpoint-config.repository';
import { SetpointConfigMapper } from '../mappers/setpoint-config.mapper';
import { EventsGateway } from '../gateways/events.gateway';
import { SetpointConfig } from '../entities/setpoint-config.entity';

@Injectable()
export class SetpointConfigService implements SetpointConfigServiceInterface {
  private readonly SOIL_TOPIC = 'greenhouse/setpoint/soil';
  private readonly AIR_TEMPERATURE_TOPIC = 'greenhouse/setpoint/air/temperature';
  private readonly SETPOINT_CONFIG_EVENT_NAME = 'setpoint-config';

  private logger = new Logger(SetpointConfigService.name);

  constructor(
    private readonly setpointConfigRepository: SetpointConfigRepository,
    @Inject(forwardRef(() => EventsGateway))
    private readonly eventsGateway: EventsGateway,
    private readonly mqttService: MqttService,
  ) {}

  async getSetpointConfig(): Promise<SetpointConfig | null> {
    return this.setpointConfigRepository.getCurrentSetpoint();
  }

  async updateSetpointConfig(msg: SetpoingConfigMessage): Promise<void> {
    if (msg.soilSetpoint === undefined && msg.airTemperatureSetpoint === undefined) {
      throw new BadRequestException('At least one setpoint field must be provided');
    }

    const existing = await this.setpointConfigRepository.getCurrentSetpoint();
    const entity = existing ?? Object.assign(new SetpointConfig(), { soilSetpoint: 70, airTemperatureSetpoint: null });
    SetpointConfigMapper.applyToEntity(msg, entity);

    try {
      await this.setpointConfigRepository.save(entity);
    } catch (err) {
      this.logger.error('Error saving setpoint config', err);
      this.eventsGateway.sendMessage(this.SETPOINT_CONFIG_EVENT_NAME, { error: (err as Error).message });
      throw err;
    }

    const publishErrors: string[] = [];

    if (msg.soilSetpoint !== undefined) {
      try {
        await this.mqttService.publish(this.SOIL_TOPIC, { soilSetpoint: msg.soilSetpoint });
        this.logger.log(`Published soil setpoint: ${msg.soilSetpoint}`);
      } catch (err) {
        this.logger.error('Failed to publish soil setpoint', err);
        publishErrors.push((err as Error).message);
      }
    }
    if (msg.airTemperatureSetpoint !== undefined) {
      try {
        await this.mqttService.publish(this.AIR_TEMPERATURE_TOPIC, { airTemperatureSetpoint: msg.airTemperatureSetpoint });
        this.logger.log(`Published air temperature setpoint: ${msg.airTemperatureSetpoint}`);
      } catch (err) {
        this.logger.error('Failed to publish air temperature setpoint', err);
        publishErrors.push((err as Error).message);
      }
    }

    if (publishErrors.length > 0) {
      this.eventsGateway.sendMessage(this.SETPOINT_CONFIG_EVENT_NAME, { error: publishErrors.join('; ') });
      return;
    }

    this.eventsGateway.sendMessage(this.SETPOINT_CONFIG_EVENT_NAME, {
      soilSetpoint: entity.soilSetpoint,
      airTemperatureSetpoint: entity.airTemperatureSetpoint,
      success: true,
    });
    this.logger.log(`Setpoint config updated: ${JSON.stringify(msg)}`);
  }
}
