import { SetpoingConfigMessage } from '../types/setpoint-config.type';
import { SetpointConfig } from '../entities/setpoint-config.entity';

export interface SetpointConfigServiceInterface {
  updateSetpointConfig(setpointConfig: SetpoingConfigMessage): Promise<void>;
  getSetpointConfig(): Promise<SetpointConfig | null>;
}
