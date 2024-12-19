#!/bin/sh

# Config
CONFIG_FILE="../infra/config/config.json"
EXPORT_CERTS="./iot_certs"

# parse env. variables
NAMESPACE=$(jq -r ".Namespace" $CONFIG_FILE)
PRJ_AWS_REGION=$(jq -r ".Env.region" $CONFIG_FILE)
PRJ_AWS_ACCOUNT=$(jq -r ".Env.account" $CONFIG_FILE)
IOT_THING_NAME=$(jq -r ".IotThingName" $CONFIG_FILE)

CERT_BUCKET=$NAMESPACE"-"$PRJ_AWS_ACCOUNT"-"$PRJ_AWS_REGION"-device-cert"
IOT_THING=$NAMESPACE"-"$IOT_THING_NAME

# clear cert dir
rm -rf $EXPORT_CERTS
mkdir -p $EXPORT_CERTS
pushd $EXPORT_CERTS

# 1. download AWS Root CA
echo "1. Download AWS Root CA"
curl -o AmazonRootCA1.pem https://www.amazontrust.com/repository/AmazonRootCA1.pem

# 2. Download iot device certs
echo "2. Download iot device certs"
aws s3 cp s3://$CERT_BUCKET/$IOT_THING/$IOT_THING.cert.pem cert.pem
aws s3 cp s3://$CERT_BUCKET/$IOT_THING/$IOT_THING.private.key private.key

# Done
ls -al
popd
echo "Done"