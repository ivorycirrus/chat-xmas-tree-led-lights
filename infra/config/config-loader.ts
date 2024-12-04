import * as fs from 'fs';
import * as path from 'path';

import { Config } from './config-type';

export class ConfigService {
  private static instance: ConfigService;
  private config: Config;

  private constructor() {
    try {
      const configPath = path.join(__dirname, './config.json');
      const configFile = fs.readFileSync(configPath, 'utf-8');
      this.config = JSON.parse(configFile) as Config;
    } catch (error) {
      throw new Error(`Fail to load config.json: ${error}`);
    }
  }

  public static getInstance(): ConfigService {
    if (!ConfigService.instance) {
      ConfigService.instance = new ConfigService();
    }
    return ConfigService.instance;
  }

  public getConfig(): Config {
    return this.config;
  }

  public get<K extends keyof Config>(key: K): Config[K] {
    return this.config[key];
  }
}
