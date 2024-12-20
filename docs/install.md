# Quick Install Guide

> * This project only has been tested on **MacBook Pro (M1 Max)** and in the AWS **us-east-1** region

## 1. Requirements

### 1.1 Application Requirements
* node.js : 20.x or higher
* python : 3.13 
* aws-cli : 2.x or higher
* boto3 : latest
* docker 25.x or higher

### 1.2 Device
* Nodemcu 1.0 (esp8266 compatible)
* WS2812B LED strib (aka. NeoPixel)
* 5V Power supply.
* 1 Register (between 200ohm to 440ohm)

### 1.3 AWS Account
Create IAM user os SSO user with IAM Identity Center.
* (option 1) If you already configured ```aws profile```
```
export AWS_PROFILE=<your-profile-name> 
```
* (option 2) With secret key and secret access key
```
export AWS_ACCESS_KEY_ID=<your-access-key>
export AWS_SECRET_ACCESS_KEY=<your-secret-access-key>
export AWS_DEFAULT_REGION=us-east-1
```

### 1.4 Bedrock Model Access
It needs to grant Amazon Bedrock Model Access for this project.
Please grant access permissions for Bedrock model what you'll use. 
This project tested on 'Anthropic Claude 3 Sonnet' and 'Anthropic Claude 3.5 Sonnet v2' .
Model access guide : [Access Amazon Bedrock foundation models](https://docs.aws.amazon.com/bedrock/latest/userguide/model-access.html)


## 2. Application Deployments

### 2.1 Project Config
Edit config files in ```/packages/infra/config```
* config.json

### 2.2 Install AWS CDK and dependencies
* Install AWS CDK with tools
```
npm install -g aws-cdk pnpm
```

* Project dependency install
```
cd packages/infra
pnpm install
```

### 2.3 (optional) CDK Bootstrap
If you have not been deployed AWS CDK application in the AWS accout, you have to bootstrap CDK Toolkit.
```
cdk bootstrap
```

### 2.4 Deploy infrastructure
```
cd packages/infra
cdk deploy --all --require-approval never
```

### 2.5 Get IoT Credentials
Download AWS IoT certs. This script download 3 files in iot_certs directory.
```
cd scripts
./download_device_certs.sh
```

Generate secret deader file for the device. This script generate secrets.h file on iot_thing/swa_iot.ino directory
```
./create_iot_secrets_header.sh
```

## 3. Device Setup
Install codes into the Nodemcu. Nodemcu is compatible with Arduino boards. And Arduino IDE for Nodemcu setup guide is [here](https://randomnerdtutorials.com/installing-esp8266-nodemcu-arduino-ide-2-0/).

### 3.1 Edit device config files in Arduino IDE
Run Arduino IDE and open iot_thing/aws_iot.ino directory.
Input your WIFI SSID and password and timezone in secrets.h file
```
const char WIFI_SSID[] = "WIFI_SSID";
const char WIFI_PASSWORD[] = "WIFI_PASSWORD";

int8_t TIME_ZONE = +9;
```

### 3.2 Edit WS2812B LED configs
Edit aws_iot.ino.ino file to edit Number of LED lights and signal pin number.
```
#define PIN D6 // DI pin number
#define N_LEDS 30 // Nexpixel LED count
```

### 3.3 Compile and install device code to Nodemcu(esp8266)
Install codes into the device. Arduino IDE setup guide is [here](https://randomnerdtutorials.com/installing-esp8266-nodemcu-arduino-ide-2-0/).
And the below library packages need to install.
* ArduinoJson
* PubSubClient
* Adafruit Neopixel

### 3.4 Flash arduino softwart on nodemcu
Flash software. 
Then, disconnect usb from tour laptop. And Next, wiring LED and others. 

## 4. Run Application
Request via HTTP Rest api to generate and run command. Put API Gateway url from setp 4.4, and set led count and query sentence as parameters.
```
curl -X POST -H "Accept: application/json" <API_GATEWAY_URL> -d '{"num_of_leds":<LED_COUNBTS>, "query": <QUERY_SENTENCE>}'
```

For example,
```
curl -X POST https://XXXXXXXXXX.execute-api.us-east-1.amazonaws.com/prod/led -H "Accept: application/json" -d '{"num_of_leds":30,"query":"I want to joyful Christmas party with SantaClaus."}'
```

## 5. Clean up
```
cd packages/infra
cdk destroy --all
```