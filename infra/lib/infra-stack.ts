import * as cdk from 'aws-cdk-lib';
import { Construct } from 'constructs';
import { ConfigService } from '../config/config-loader';

export class InfraStack extends cdk.Stack {
    constructor(scope: Construct, id: string, props?: cdk.StackProps) {
        super(scope, id, props);

        // Load config values
        const Config = ConfigService.getInstance().getConfig();

    }
}
