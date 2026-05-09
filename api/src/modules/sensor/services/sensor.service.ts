import { Injectable, Logger } from '@nestjs/common';
import { HistoricalDataPoint, HistoricalDataResponse, SensorMessage } from '../types/sensor.type';
import { SensorServiceInterface } from './sensor.service.interface';
import { EventsGateway } from '../gateways/events.gateway';
import { SensorDataRepository } from '../repositories/sensor-data.repository';
import { RawBucketRow } from '../repositories/sensor-data.repository.interface';
import { SensorMapper } from '../mappers/sensor.mapper';

@Injectable()
export class SensorService implements SensorServiceInterface {
  private SENSOR_EVENT_NAME = 'sensor-update';
  private logger = new Logger(SensorService.name);

  constructor(
    private readonly sensorDataRepository: SensorDataRepository,
    private readonly eventsGateway: EventsGateway,
  ) {}

  async handleSensorData(data: SensorMessage): Promise<void> {
    if (data.sensor_data_moment_id) {
      const duplicate = await this.sensorDataRepository.existsByMomentId(data.sensor_data_moment_id);
      if (duplicate) {
        this.logger.warn(`Duplicate sensor_data_moment_id ${data.sensor_data_moment_id} — skipping`);
        return;
      }
    }

    try {
      this.logger.log(`Saving sensor data: ${JSON.stringify(data)}`);
      const sensorData = SensorMapper.toEntity(data);
      await this.sensorDataRepository.save(sensorData);
      this.logger.log('Sensor data saved successfully');
    } catch (err) {
      this.logger.error('Error saving sensor data', err);
      this.eventsGateway.sendMessage(this.SENSOR_EVENT_NAME, {
        error: (err as Error).message,
      });
      throw err;
    }

    this.logger.log('Sending sensor data');
    this.eventsGateway.sendMessage<SensorMessage>(this.SENSOR_EVENT_NAME, data);
    this.logger.log('Sensor data sent successfully');
  }

  async getHistoricalData(hours: number): Promise<HistoricalDataResponse> {
    const rows = await this.sensorDataRepository.findByTimeRange(hours);
    return this.buildResponse(rows);
  }

  async getHistoricalDataByRange(from: Date, to: Date): Promise<HistoricalDataResponse> {
    const rows = await this.sensorDataRepository.findByDateRange(from, to);
    return this.buildResponse(rows);
  }

  private buildResponse(rows: RawBucketRow[]): HistoricalDataResponse {
    const points: HistoricalDataPoint[] = rows.map((row) => ({
      timestamp: new Date(row.bucket),
      air: {
        temperature: row.air_temperature !== null ? Number(row.air_temperature) : null,
        humidity:    row.air_humidity    !== null ? Number(row.air_humidity)    : null,
      },
      soil: {
        humidity: row.soil_humidity !== null ? Number(row.soil_humidity) : null,
      },
    }));

    let sumTemp = 0, countTemp = 0;
    let sumHum  = 0, countHum  = 0;
    let sumSoil = 0, countSoil = 0;

    for (const p of points) {
      if (p.air.temperature !== null) { sumTemp += p.air.temperature; countTemp++; }
      if (p.air.humidity    !== null) { sumHum  += p.air.humidity;    countHum++;  }
      if (p.soil.humidity   !== null) { sumSoil += p.soil.humidity;   countSoil++; }
    }

    return {
      points,
      average: {
        air: {
          temperature: countTemp > 0 ? sumTemp / countTemp : null,
          humidity:    countHum  > 0 ? sumHum  / countHum  : null,
        },
        soil: {
          humidity: countSoil > 0 ? sumSoil / countSoil : null,
        },
      },
    };
  }
}
