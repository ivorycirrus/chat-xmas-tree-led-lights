#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <time.h>
#include "secrets.h"
#define TIME_ZONE +9
 
float h ;
float t;
unsigned long lastMillis = 0;
unsigned long previousMillis = 0;
const long interval = 5000;
 
#define AWS_IOT_PUBLISH_TOPIC   "/esp8266/pub"
#define AWS_IOT_SUBSCRIBE_TOPIC "/esp8266/sub"
 
WiFiClientSecure net;
 
BearSSL::X509List cert(cacert);
BearSSL::X509List client_crt(client_cert);
BearSSL::PrivateKey key(privkey);
 
PubSubClient client(net);
 
time_t now;
time_t nowish = 1510592825;

int** cmd;
int cmd_len = 0;
int** cmd_candidate;
int cmd_candidate_len = 0;
bool cmd_invalidate = true;

const int CMD_SET = 1;
const int CMD_SLEEP = 2;
const int CMD_REPEAT = 3;

//--------------------------------------
// Datetime initialize
//--------------------------------------
void NTPConnect(void)
{
  Serial.print("Setting time using SNTP");
  configTime(TIME_ZONE * 3600, 0 * 3600, "pool.ntp.org", "time.nist.gov");
  now = time(nullptr);
  while (now < nowish)
  {
    delay(500);
    Serial.print(".");
    now = time(nullptr);
  }
  Serial.println("done!");
  struct tm timeinfo;
  gmtime_r(&now, &timeinfo);
  Serial.print("Current time(GST): ");
  Serial.print(asctime(&timeinfo));
}
 
//--------------------------------------
// MQTT handlers 
//--------------------------------------
void messageReceived(char *topic, byte *payload, unsigned int length)
{
  Serial.print("Received [");
  Serial.print(topic);
  Serial.println("]: ");

  DynamicJsonDocument doc(4096);
  DeserializationError error = deserializeJson(doc, payload, length);

  JsonArray dataArray = doc["data"];

  int i = 0;
  for (JsonObject obj : dataArray) {
    String cmd = obj["cmd"].as<String>();
    Serial.print("Command: ");
    Serial.print(cmd);

    // parse commands
    if (cmd.equals("set")) {
      int from = obj["led_id_from"];
      int to = obj["led_id_to"];
      int r = obj["color"][0];
      int g = obj["color"][1];
      int b = obj["color"][2];
      Serial.print("  from: ");
      Serial.print(from);
      Serial.print(" >>  to: ");
      Serial.println(to);
      cmd_candidate[i][0] = CMD_SET;
      cmd_candidate[i][1] = from;
      cmd_candidate[i][2] = to;
      cmd_candidate[i][3] = r;
      cmd_candidate[i][4] = g;
      cmd_candidate[i][5] = b;
      i++;
    } 
    else if (cmd.equals("sleep")) {
      int sleep_duration = obj["milis"];
      Serial.print("  Sleep duration: ");
      Serial.println(sleep_duration);
      cmd_candidate[i][0] = CMD_SLEEP;
      cmd_candidate[i][1] = sleep_duration;
      i++;
    } 
    else if (cmd.equals("repeat")) {
      bool repeat = obj["repeat"];
      Serial.print("  Repeat: ");
      Serial.println(repeat ? "true" : "false");
      cmd_candidate[i][0] = CMD_REPEAT;
      cmd_candidate[i][1] = repeat ? 1 : 0;
      i++;
    }
  }
  cmd_candidate_len = i;
  cmd_invalidate = true;
}

void publishMessage(String data)
{
  StaticJsonDocument<200> doc;
  doc["time"] = now;
  doc["data"] = data;
  char jsonBuffer[512];
  serializeJson(doc, jsonBuffer); // print to client
 
  client.publish(AWS_IOT_PUBLISH_TOPIC, jsonBuffer);
}
 
//--------------------------------------
// Connected to AWS IoT
//--------------------------------------
void connectAWS()
{
  delay(3000);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
 
  Serial.println(String("Attempting to connect to SSID: ") + String(WIFI_SSID));
 
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(1000);
  }
 
  NTPConnect();
 
  net.setTrustAnchors(&cert);
  net.setClientRSACert(&client_crt, &key);
 
  client.setServer(MQTT_HOST, 8883);
  client.setCallback(messageReceived);
 
 
  Serial.println("Connecting to AWS IOT");
 
  while (!client.connect(THINGNAME))
  {
    Serial.print(".");
    delay(1000);
  }
 
  if (!client.connected()) {
    Serial.println("AWS IoT Timeout!");
    return;
  }
  // Subscribe to a topic
  client.subscribe(AWS_IOT_SUBSCRIBE_TOPIC);
 
  Serial.println("AWS IoT Connected!");
}

//--------------------------------------
// Command utils
//--------------------------------------
void resetCommand() {
  cmd_len = 0;
  cmd_candidate_len = 0;
  cmd = new int*[100];
  cmd_candidate = new int*[100];
  for(int i = 0; i < 100; i++) {
    cmd[i] = new int[6];
    cmd_candidate[i] = new int[6];
    for(int j = 0; j < 6; j++) {
      cmd[i][j] = 0;
      cmd_candidate[i][j] = 0;
    }
  }
}

void updateCommand() {
  Serial.println("Update command - init");
  // update commands
  for(int i = 0; i < cmd_candidate_len; i++) {
    for(int j = 0; j < 6; j++) {
      cmd[i][j] = cmd_candidate[i][j];
    }
  }
  // reset cmd length
  cmd_len = cmd_candidate_len;
  cmd_candidate_len = 0;
  // update fnish
  cmd_invalidate = false;
  Serial.print("Update command - done(");
  Serial.print(cmd_len);
  Serial.println(")");
}
 
//--------------------------------------
// Ardunio codes body
//--------------------------------------
void setup()
{
  Serial.begin(115200);
  resetCommand();
  connectAWS();
}
 
void loop()
{
  now = time(nullptr);
  Serial.print("Now: ");
  Serial.println(now);
 
  if (!client.connected())
  {
    connectAWS();
  }
  else
  {
    // AWS IoT connectivity
    client.loop();

    // Message processing
    if(cmd_invalidate) updateCommand();

    // Command execution    
    for(int seq = 0 ; seq < cmd_len ; seq++) {
      Serial.print("EXEC[");
      Serial.print(seq);
      Serial.print("/");
      Serial.print(cmd_len);
      Serial.print("] > ");
      delay(1000);

      if(cmd[seq][0] == CMD_SET) {
        Serial.print(" set ");
        Serial.print("  from: ");
        Serial.print(cmd[seq][1]);
        Serial.print(" - to: ");
        Serial.print(cmd[seq][2]);
        Serial.print("  - RGB: ");
        Serial.print(cmd[seq][3]);
        Serial.print(", ");
        Serial.print(cmd[seq][4]);
        Serial.print(", ");
        Serial.println(cmd[seq][5]);
        // TO-DO : implement set command
      } else if(cmd[seq][0] == CMD_SLEEP) {
        Serial.print(" sleep ");
        Serial.println(cmd[seq][1]);
        delay(cmd[seq][1]);

        // ckeck new message after sleep
        client.loop();
        // interrupt when new message arrived
        if(cmd_invalidate) break;
      } 
    }

    // Dummy for nothing to execute
    if(cmd_len == 0) {
      // wait for next comment checkup
      delay(3000);
    }
    // Repeatation set-up after all command list retrieved
    else if(cmd[cmd_len-1][0] == CMD_REPEAT) {
      // repeat false => cmd length set to 0      
      Serial.print("EXEC : repeat set to ");
      Serial.println(cmd[cmd_len-1][1]);
      if(cmd[cmd_len-1][1] == 0) cmd_len = 0;
    }

    // Health check
    if (millis() - lastMillis > 30000)
    {
      lastMillis = millis();
      publishMessage("Health check");
      Serial.println("Health check.");
    }

    // Loop safeguard
    delay(1000);
  }
}