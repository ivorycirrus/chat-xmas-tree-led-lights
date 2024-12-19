#!/bin/sh

# Taeget file
SECRETS_HEADER_FILE="../iot_thing/aws_iot.ino/secrets.h"
echo "Create IoT Thing Certs : $SECRETS_HEADER_FILE

# Config
CONFIG_FILE="../infra/config/config.json"
EXPORT_CERTS="./iot_certs"

# device settings
TIME_ZONE="+9"
WIFI_SSID="WIFI_SSID"
WIFI_PASSWORD="WIFI_PASSWORD"
MQTT_HOST=$(aws iot describe-endpoint --endpoint-type iot:Data-ATS | jq -r ".endpointAddress" -)

# parse env. variables
NAMESPACE=$(jq -r ".Namespace" $CONFIG_FILE)
PRJ_AWS_REGION=$(jq -r ".Env.region" $CONFIG_FILE)
PRJ_AWS_ACCOUNT=$(jq -r ".Env.account" $CONFIG_FILE)
IOT_THING_NAME=$(jq -r ".IotThingName" $CONFIG_FILE)
IOT_THING=$NAMESPACE"-"$IOT_THING_NAME

# export secrets header
echo "#include <pgmspace.h>" > $SECRETS_HEADER_FILE
echo "" >> $SECRETS_HEADER_FILE
echo "#define SECRET" >> $SECRETS_HEADER_FILE
echo "const char WIFI_SSID[] = \""$WIFI_SSID"\";" >> $SECRETS_HEADER_FILE
echo "const char WIFI_PASSWORD[] = \""$WIFI_PASSWORD"\";" >> $SECRETS_HEADER_FILE
echo "" >> $SECRETS_HEADER_FILE
echo "#define THINGNAME \"$IOT_THING\"" >> $SECRETS_HEADER_FILE
echo "int8_t TIME_ZONE = "$TIME_ZONE";" >> $SECRETS_HEADER_FILE
echo "const char MQTT_HOST[] = \""$MQTT_HOST"\";" >> $SECRETS_HEADER_FILE
echo "" >> $SECRETS_HEADER_FILE
echo "static const char cacert[] PROGMEM = R\"EOF(" >> $SECRETS_HEADER_FILE
cat $EXPORT_CERTS/AmazonRootCA1.pem >> $SECRETS_HEADER_FILE
echo ")EOF\";" >> $SECRETS_HEADER_FILE
echo "" >> $SECRETS_HEADER_FILE
echo "static const char client_cert[] PROGMEM = R\"KEY(" >> $SECRETS_HEADER_FILE
cat $EXPORT_CERTS/cert.pem >> $SECRETS_HEADER_FILE
echo ")KEY\";" >> $SECRETS_HEADER_FILE
echo "" >> $SECRETS_HEADER_FILE
echo "static const char privkey[] PROGMEM = R\"KEY(" >> $SECRETS_HEADER_FILE
cat $EXPORT_CERTS/private.key >> $SECRETS_HEADER_FILE
echo ")KEY\";" >> $SECRETS_HEADER_FILE
echo "" >> $SECRETS_HEADER_FILE

# Done
echo "Done"