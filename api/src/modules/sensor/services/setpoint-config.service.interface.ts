import { SetpoingConfigMessage } from '../types/setpoint-config.type';

export interface SetpointConfigServiceInterface {
  updateSetpointConfig(setpointConfig: SetpoingConfigMessage): Promise<void>;
}
