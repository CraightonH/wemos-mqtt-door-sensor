#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <PubSubClient.h>

const int SWITCHPIN = D4;

String devID = "esp8266-door-sensor";
const char* doorTopic = "/garage/door";
const char* logInfoTopic = "/log/info";
bool prevDoorClosed = false;
bool doorClosed = false;
long timer = 0;

WiFiClient wifiClient;
PubSubClient client("192.168.1.239", 1883, wifiClient);

void findKnownWiFiNetworks() {
  ESP8266WiFiMulti wifiMulti;
  WiFi.mode(WIFI_STA);
  wifiMulti.addAP("BYU-WiFi", "");
  wifiMulti.addAP("Hancock2.4G", "Arohanui");
  Serial.println("");
  Serial.print("Connecting to Wifi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    wifiMulti.run();
    delay(1000);
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(WiFi.SSID().c_str());
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void pubDoorStatePeriodic(bool doorClosed) {
  if (millis() > timer + 5000) {
    timer = millis();
    pubDoorState(doorTopic, doorClosed);
  }
}

void pubDoorState(const char* topic, bool doorClosed) {
    String door = "open";
    if (!doorClosed) {
      door = "closed";  
    }
    client.publish(topic, door.c_str());
}

void pubDebug(String message) {
  client.publish("/log/debug", message.c_str());
}

void sendHassDeviceConfig() {
  client.publish("homeassistant/sensor/garage/door_sensor/config", "{\"name\": \"Garage Door\", \"state_topic\": \"/garage/door\"}");
}

void reconnectMQTT() {
  while(!client.connected()) {
    if (client.connect((char*) devID.c_str(), "mqtt", "mymqttpassword")) {
      Serial.println("Connected to MQTT server");
      String message = devID;
      message += ": ";
      message += WiFi.localIP().toString();
      if (client.publish(logInfoTopic, message.c_str())) {
        Serial.println("published successfully");
      } else {
        Serial.println("failed to publish");
      }
      sendHassDeviceConfig();
    } else {
      Serial.println("MQTT connection failed");
      delay(5000);
    }
  }
}

void setup(void) {
  Serial.begin(115200);
  findKnownWiFiNetworks();
  pinMode(SWITCHPIN, INPUT);
}

void loop(void) {
  if (!client.connected()) {
    reconnectMQTT();
  }
  client.loop();
  if (digitalRead(SWITCHPIN) == LOW) {
    doorClosed = false;
  } else {
    doorClosed = true;  
  }
  if (prevDoorClosed != doorClosed) {
    prevDoorClosed = doorClosed;
    pubDoorState(doorTopic, doorClosed);  
  }
  pubDoorStatePeriodic(doorClosed);
}
