import { SetpointConfig } from '../entities/setpoint-config.entity';
import { SetpoingConfigMessage } from '../types/setpoint-config.type';

export class SetpointConfigMapper {
  static applyToEntity(msg: SetpoingConfigMessage, entity: SetpointConfig): void {
    if (msg.soilSetpoint !== undefined) entity.soilSetpoint = msg.soilSetpoint;
    if (msg.airTemperatureSetpoint !== undefined) entity.airTemperatureSetpoint = msg.airTemperatureSetpoint;
  }
}
