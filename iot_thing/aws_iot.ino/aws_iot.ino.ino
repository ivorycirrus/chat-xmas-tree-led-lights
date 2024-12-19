// esp8266 connectivity 
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>

// for AWS IoT core
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <time.h>

// for WS2812B LED strib
#include <Adafruit_NeoPixel.h>

// configurations
#include "secrets.h"
#define TIME_ZONE +9

// system clock
unsigned long lastMillis = 0;
unsigned long previousMillis = 0;
const long interval = 5000;
time_t now;
time_t nowish = 1510592825;

// AWS IoT Core - MQTT topics 
#define AWS_IOT_PUBLISH_TOPIC   "/esp8266/pub" // to AWS
#define AWS_IOT_SUBSCRIBE_TOPIC "/esp8266/sub" // from AWS
 
WiFiClientSecure net;
BearSSL::X509List cert(cacert);
BearSSL::X509List client_crt(client_cert);
BearSSL::PrivateKey key(privkey);
PubSubClient client(net);

// LED command store 
int** cmd;
int cmd_len = 0;
int** cmd_candidate;
int cmd_candidate_len = 0;
bool cmd_invalidate = true;

// Command definition
const int CMD_SET = 1;
const int CMD_SLEEP = 2;
const int CMD_REPEAT = 3;

// LED setup
#define PIN D6 // DI pin number
#define N_LEDS 30 // Nexpixel LED count
#define BRIGHTNESS 50
Adafruit_NeoPixel WS2812B = Adafruit_NeoPixel(N_LEDS, PIN, NEO_GRB + NEO_KHZ800); 

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
  Serial.println(length);

  DynamicJsonDocument doc(8192);
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
 
  client.setBufferSize(4096);
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
  publishMessage("LED device connected");
}

//--------------------------------------
// Command utils
//--------------------------------------
void resetCommand() {
  cmd_len = 0;
  cmd_candidate_len = 0;
  cmd = new int*[50];
  cmd_candidate = new int*[50];
  for(int i = 0; i < 50; i++) {
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
  publishMessage("Command updated");
}

//--------------------------------------
// LED functions
//--------------------------------------
void initLEDs() {
  WS2812B.begin(); // INITIALIZE WS2812B strip object (REQUIRED)
  WS2812B.setBrightness(BRIGHTNESS);  // brightness setup(RGBW only)
  WS2812B.clear(); 
  WS2812B.show();
}

void setLEDs(int from, int to, int red, int green, int blue) {
  Serial.print(" set ");
  Serial.print("  from: ");
  Serial.print(from);
  Serial.print(" - to: ");
  Serial.print(to);
  Serial.print("  - RGB: ");
  Serial.print(red);
  Serial.print(", ");
  Serial.print(green);
  Serial.print(", ");
  Serial.println(blue);
  for(int i = from ; i <= to ; i++) {
    WS2812B.setPixelColor(i, WS2812B.Color(red, green, blue)); 
  }
}
 
//--------------------------------------
// Ardunio codes body
//--------------------------------------
void setup()
{
  Serial.begin(115200);
  resetCommand();
  connectAWS();
  initLEDs();
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
        // set led
        setLEDs(cmd[seq][1], cmd[seq][2], cmd[seq][3], cmd[seq][4], cmd[seq][5]);
      } else if(cmd[seq][0] == CMD_SLEEP) {
        Serial.print(" sleep ");
        Serial.println(cmd[seq][1]);

        // show
        WS2812B.show(); 
        // sleep
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
    if (millis() - lastMillis > 60000)
    {
      lastMillis = millis();
      publishMessage("Health check");
      Serial.println("Health check.");
    }

    // Loop safeguard
    delay(1000);
  }
}