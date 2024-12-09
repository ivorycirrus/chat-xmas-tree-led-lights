import {
    Stack,
    StackProps,
    RemovalPolicy,
    aws_s3 as s3,
} from 'aws-cdk-lib';
import { Construct } from 'constructs';
import { ConfigService } from '../config/config-loader';
import { ThingWithCert } from 'cdk-iot-core-certificates-v3';

export class InfraStack extends Stack {
    constructor(scope: Construct, id: string, props?: StackProps) {
        super(scope, id, props);

        // Load config values
        const Config = ConfigService.getInstance().getConfig();

        // cert bucket
        console.log(Stack.of(this).account)
        const deviceCertBucketName = `${Config.Namespace}-${Config.Env.account}-${Config.Env.region}-device-cert`;
        const deviceCertBucket = this.createS3Bucket(deviceCertBucketName);

        // create iot thing
        const { thingArn, certId, certPem, privKey } = new ThingWithCert(this, 'IotThing', {
            thingName: `${Config.Namespace}-${Config.IotThingName}`,
            saveFileBucket: deviceCertBucket,
        });
    }

    private createS3Bucket(bucketName: string) {
        return new s3.Bucket(this, bucketName, {
            bucketName: bucketName,
            // encryption
            encryption: s3.BucketEncryption.S3_MANAGED,
            blockPublicAccess: s3.BlockPublicAccess.BLOCK_ALL,
            enforceSSL: true,
            // log
            serverAccessLogsPrefix: 'logs/',
            // delete ovject policy
            removalPolicy: RemovalPolicy.DESTROY,
            autoDeleteObjects: true,
        });
    }
}
