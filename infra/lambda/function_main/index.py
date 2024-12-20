import os
import json
import boto3
from led_command import LedCommandService

BEDROCK_MODEL_ID = os.getenv('BEDROCK_MODEL_ID', 'anthropic.claude-3-sonnet-20240229-v1:0')
IOT_COMMAND_TOPIC = os.getenv('IOT_COMMAND_TOPIC', '/esp8266/sub')

# LLM Service
service = LedCommandService(BEDROCK_MODEL_ID)

# AWS IoT Core 엔드포인트 설정
def get_iot_endpoint():
    iot_client = boto3.client('iot')
    result = iot_client.describe_endpoint(endpointType='iot:Data-ATS')
    return result['endpointAddress']

# IoT Data Client 생성
IOT_ENDPOINT = get_iot_endpoint()
iot_client = boto3.client('iot-data', endpoint_url=f"https://{IOT_ENDPOINT}")

# /esp8266/sub 토픽으로 메시지 발행
def send_to_iot(data):
    try:
        response = iot_client.publish(
            topic='/esp8266/sub',
            qos=1,
            payload=data
        )
        return {
            'statusCode': 200,
            'body': 'Message published successfully'
        }
    except Exception as e:
        return {
            'statusCode': 500,
            'body': f'Error publishing message: {str(e)}'
        }

def handler(event, context):

    # parse query from http body 
    req = None
    if 'body' in event and event['body'] is not None:
        req = json.loads(event['body'])

    print(req)
    
    result = { "error" : "invalid request" }
    if req is not None and 'query' in req and 'num_of_leds' in req:
        #result = service.led_command_with_planning_agent(req['num_of_leds'], req['query'])
        result = service.simple_led_command(req['num_of_leds'], req['query'])
        data = json.dumps(result)
        print(data)
        send_to_iot(data)

    api_response = {
        'statusCode': 200,
        'headers': {
            'Access-Control-Allow-Origin': '*',
            'Content-Type': 'application/json',
        },
        'body': data
    }
    return api_response