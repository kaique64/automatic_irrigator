import { SetpointConfig } from '../entities/setpoint-config.entity';

export interface SetpointConfigRepositoryInterface {
  save(setpointConfig: SetpointConfig): Promise<void>;
  getCurrentSetpoint(): Promise<SetpointConfig | null>;
}
