import { SetpointConfig } from '../entities/setpoint-config.entity';
import { SetpoingConfigMessage } from '../types/setpoint-config.type';

export class SetpointConfigMapper {
  static toEntity({ setpoint }: SetpoingConfigMessage): SetpointConfig {
    const setpointConfig = new SetpointConfig();

    setpointConfig.setpoint = setpoint;

    return setpointConfig;
  }
}
