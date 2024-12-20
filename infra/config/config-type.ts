export interface Config {
    Namespace: string;
    Env: Record<string,string>;
    BedrockModelId: string;
    IotThingName: string;
    IoTCommandTopic: string;
}