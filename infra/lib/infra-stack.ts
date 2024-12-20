import {
    Stack,
    StackProps,
    RemovalPolicy,
    Duration,
    aws_iam as iam,
    aws_s3 as s3,
    aws_lambda as lambda,
    aws_apigateway as apigateway,
    CfnOutput,
} from 'aws-cdk-lib';
import { Construct } from 'constructs';
import { PythonLayerVersion } from '@aws-cdk/aws-lambda-python-alpha';
import { ConfigService } from '../config/config-loader';
import { ThingWithCert } from 'cdk-iot-core-certificates-v3';

export class InfraStack extends Stack {
    constructor(scope: Construct, id: string, props?: StackProps) {
        super(scope, id, props);

        // Load config values
        const Config = ConfigService.getInstance().getConfig();

        // cert bucket
        const deviceCertBucketName = `${Config.Namespace}-${Config.Env.account}-${Config.Env.region}-device-cert`;
        const deviceCertBucket = this.createS3Bucket(deviceCertBucketName);

        // create iot thing
        const { thingArn, certId, certPem, privKey } = new ThingWithCert(this, 'IotThing', {
            thingName: `${Config.Namespace}-${Config.IotThingName}`,
            saveFileBucket: deviceCertBucket,
        });

        // Create lambda function to generate LED commands
        const lambdaLayer = this.createLangChainLambdaLayer();
        const lambdaFunction = this.createLedCommandLambdaFunction([lambdaLayer]);

        // API Gateway
        const apigw = new apigateway.LambdaRestApi(this, 'RestApi', {
            restApiName: "Generate LED Commands API",
            handler: lambdaFunction,
            proxy: false,
        });
        const ledApi = apigw.root.addResource('led');
        ledApi.addMethod('POST', new apigateway.LambdaIntegration(lambdaFunction));
        new CfnOutput(this, 'ApiEndpoint', { value: apigw.url, description: 'API endpoint', exportName: 'ApiEndpoint' });
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

    private createLangChainLambdaLayer() {
        // Load config values
        const Config = ConfigService.getInstance().getConfig();

        return new PythonLayerVersion(this, 'LangChainLayer', {
            layerVersionName: `${Config.Namespace}-langchain-layer`,
            entry: 'lambda/layer_lib',
            compatibleRuntimes: [
                lambda.Runtime.PYTHON_3_13,
            ],
        });
    }

    private createLedCommandLambdaFunction(layers: lambda.ILayerVersion[]) {
        // Load config values
        const Config = ConfigService.getInstance().getConfig();

        // add lambda function
        const funcRole = new iam.Role(this, 'LambdaFunctionRole', {
            assumedBy: new iam.ServicePrincipal('lambda.amazonaws.com'),
            inlinePolicies:{
                "LambdaInvoke": new iam.PolicyDocument({
                    statements: [
                        new iam.PolicyStatement({
                            effect: iam.Effect.ALLOW,
                            actions: [
                                // Lambda default execution role
                                "logs:CreateLogGroup",
                                "logs:CreateLogStream",
                                "logs:PutLogEvents",
                                // Lambda layer
                                "lambda:GetLayerVersion",
                                // Bedrock invoke
                                "bedrock:InvokeModel",
                                // IoT publish
                                "iot:DescribeEndpoint",
                                "iot:Publish",
                            ],
                            resources: ['*']
                        })
                    ]
                })
            }
        });
        const lambdaFunction = new lambda.Function(this, 'LambdaFunction', {
            functionName: `${Config.Namespace}-create-led-command-and-show`,
            code: lambda.Code.fromAsset('lambda/function_main'),
            handler: 'index.handler',
            runtime: lambda.Runtime.PYTHON_3_13,
            memorySize: 2048,
            timeout: Duration.minutes(5),
            layers: layers,
            environment: {
                BEDROCK_MODEL_ID: Config.BedrockModelId,
                IOT_COMMAND_TOPIC: Config.IoTCommandTopic,
            },
            description: 'Create LED control command from user input, and sent to iot device',
            role: funcRole
        });

        return lambdaFunction;
    }
}
