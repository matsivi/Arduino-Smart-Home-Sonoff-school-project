#include "config.h"  
#include <WiFi.h>
#include <HTTPClient.h>
#include <time.h>

#define PIR_PIN 13
#define LDR_PIN 35
#define MQ135_PIN 34  // Π.χ. GPIO 34 για analog in

const char* ssid = "COSMOTE-955376";
const char* password = "123456789";

// --- Channel A: PIR, Φως (PIR/LED events) ---
const char* channelA_ApiKey = "RX97HML2AG46RLYM";
const char* channelA_Url = "http://api.thingspeak.com/update?api_key=";

// --- Channel B: MQ135, Mode ---
const char* channelB_ApiKey = "A7W8HEL5G4B3FTFY";
const char* channelB_Url = "http://api.thingspeak.com/update?api_key=";

// Poll για mode switch 
const char* thingspeakReadAPIKey = "BP3A832DD4TIAIXR";
const char* thingspeakPollUrl = "http://api.thingspeak.com/channels/3004952/fields/1/last.json?api_key=BP3A832DD4TIAIXR";

// Timing
unsigned long lastModeCheck = 0;
const unsigned long modeCheckInterval = 10000UL;
int currentSensorMode = 1;  // 1=WiFi, 2=LoRa
unsigned long lastDataSend = 0;
const unsigned long dataSendInterval = 30000UL;
unsigned long lastField4Update = 0;
int field4Value = 1;
const unsigned long field4Interval = 120000UL;


void sendOffToSonoff() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.setTimeout(4000);
    String url = "https://c764aa149782.ngrok-free.app/zeroconf/switch";
    String payload = "{\"deviceid\":\"1000b913b0\",\"data\":{\"switch\":\"off\"}}";
    http.begin(url);
    http.addHeader("Content-Type", "application/json");
    int code = http.POST(payload);
    http.end();
    Serial.print("Sent OFF to Sonoff | HTTP code: ");
    Serial.println(code);
  }
}


void sendPirEventToThingSpeak(int pirVal) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.setTimeout(4000);
    String url = String(channelA_Url) + channelA_ApiKey + "&field1=" + String(pirVal);
    http.begin(url);
    int code = http.GET();
    http.end();
    Serial.print("Sent PIR event (Channel A): ");
    Serial.print(pirVal);
    Serial.print(" | HTTP code: ");
    Serial.println(code);
  }
}

int pollSensorModeFromThingSpeak() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.setTimeout(4000);
    http.begin(thingspeakPollUrl);
    int code = http.GET();
    int mode = currentSensorMode;
    if (code == 200) {
      String payload = http.getString();
      int idx = payload.indexOf("\"field1\":\"");
      if (idx >= 0) {
        int valueStart = idx + 10;
        String rest = payload.substring(valueStart);
        int endIdx = rest.indexOf("\"");
        String val = rest.substring(0, endIdx);
        int modeVal = val.toInt();
        if (modeVal == 1 || modeVal == 2) mode = modeVal;
      }
    }
    http.end();
    return mode;
  }
  return currentSensorMode;
}

void sendMessageToThingspeak(int value) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.setTimeout(4000);
    String url = String(channelB_Url) + channelB_ApiKey + "&field1=" + String(value);
    http.begin(url);
    int code = http.GET();
    http.end();
    Serial.print("Sent MODE (1/2) to ThingSpeak (Channel B, field1): ");
    Serial.print(value);
    Serial.print(" | HTTP code: ");
    Serial.println(code);
  }
}


void sendMq135ThingSpeak(int value) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.setTimeout(4000);
    String url = String(channelB_Url) + channelB_ApiKey + "&field2=" + String(value);
    http.begin(url);
    int code = http.GET();
    http.end();
    Serial.print("Sent MQ135 to ThingSpeak (Channel B) | HTTP code: ");
    Serial.println(code);
  }
}

void sendMq135LoRa(int value) {
  time_t timestamp = time(nullptr);  
  uint8_t uplinkPayload[6];
  uplinkPayload[0] = highByte(value);
  uplinkPayload[1] = lowByte(value);
  uplinkPayload[2] = (timestamp >> 24) & 0xFF;
  uplinkPayload[3] = (timestamp >> 16) & 0xFF;
  uplinkPayload[4] = (timestamp >> 8) & 0xFF;
  uplinkPayload[5] = (timestamp)&0xFF;

  Serial.print(F("LoRa Payload: "));
  arrayDump(uplinkPayload, 6);
  Serial.print("Timestamp sent: ");
  Serial.println(timestamp);
  int16_t state = node.sendReceive(uplinkPayload, sizeof(uplinkPayload));
  if (state < RADIOLIB_ERR_NONE)
    Serial.print("Error in sendReceive: "), Serial.println(state);
  else if (state > 0)
    Serial.println(F("Downlink received (LoRa)"));
  else
    Serial.println(F("No downlink (LoRa)"));
}



void setup() {
  Serial.begin(115200);
  pinMode(PIR_PIN, INPUT);
  pinMode(LDR_PIN, INPUT);
  pinMode(MQ135_PIN, INPUT);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");
  Serial.print("ESP IP: ");
  Serial.println(WiFi.localIP());

  // NTP time sync για σωστό epoch
  configTime(2 * 3600, 0, "pool.ntp.org");  // Greece: GMT+2
  Serial.println("Waiting for NTP time...");
  time_t now = time(nullptr);
  while (now < 1700000000) {  
    delay(500);
    Serial.print(".");
    now = time(nullptr);
  }
  Serial.println("\nNTP time set.");

  mySPI.begin(5, 19, 27, 18);
  int16_t state = radio.begin();
  if (state != RADIOLIB_ERR_NONE) {
    Serial.print("LoRa radio init failed: ");
    Serial.println(state);
    while (1) delay(1000);
  }
  state = node.beginOTAA(joinEUI, devEUI, nwkKey, appKey);
  if (state != RADIOLIB_ERR_NONE) {
    Serial.print("LoRa node OTAA failed: ");
    Serial.println(state);
    while (1) delay(1000);
  }
  state = node.activateOTAA();
  if (state != RADIOLIB_LORAWAN_NEW_SESSION) {
    Serial.print("LoRa join failed: ");
    Serial.println(state);
    while (1) delay(1000);
  }
  Serial.println(F("LoRa ready!\n"));
}

void loop() {
  unsigned long now = millis();

  if (now - lastModeCheck >= modeCheckInterval) {
    lastModeCheck = now;
    int polledMode = pollSensorModeFromThingSpeak();
    if (polledMode != currentSensorMode) {
      currentSensorMode = polledMode;
      Serial.print("Mode switched! Now: ");
      Serial.println(currentSensorMode == 1 ? "WiFi/ThingSpeak" : "LoRa/TTN");
    }
  }

  if (now - lastDataSend >= dataSendInterval) {
    lastDataSend = now;
    int mq135Value = analogRead(MQ135_PIN);
    if (currentSensorMode == 1) {
      sendMq135ThingSpeak(mq135Value);
      Serial.print("SENT: MQ135 (WiFi/ThingSpeak, Channel B) — Value: ");
      Serial.println(mq135Value);
    }
    if (currentSensorMode == 2) {
      sendMq135LoRa(mq135Value);
      Serial.print("SENT: MQ135 (LoRa/TTN) — Value: ");
      Serial.println(mq135Value);
    }
  }

  // === PIR/LDR/LED LOGIC ===
  static int lastLdrValue = -1;
  static bool lastPirState = LOW;
  static bool ledIsOn = false;
  static unsigned int motionCounter = 0;

  int ldrValue = digitalRead(LDR_PIN);
  bool isNight = (ldrValue == HIGH);

  if (ldrValue != lastLdrValue) {
    lastLdrValue = ldrValue;
    Serial.print("LDR CHANGED! Νέα τιμή: ");
    Serial.print(ldrValue);
    Serial.print(" | isNight: ");
    Serial.println(isNight ? "YES" : "NO");
    if (!isNight) {
      sendOffToSonoff();  // νέο!
      Serial.println("Ξημέρωσε — LED OFF!");
    }
  }

  // PIR trigger μόνο τη νύχτα (και αν δεν είναι ήδη ON)
  bool pirValue = digitalRead(PIR_PIN);
  if (isNight && pirValue != lastPirState) {
    lastPirState = pirValue;
    if (pirValue == HIGH) {
      motionCounter++;
      int eventToSend = motionCounter % 2;
      sendPirEventToThingSpeak(eventToSend);
      Serial.print("Motion + Night! PIR event (sent): ");
      Serial.println(eventToSend);
    }
  }

  if (now - lastField4Update >= field4Interval) {
    lastField4Update = now;
    sendMessageToThingspeak(field4Value);
    field4Value = (field4Value == 1) ? 2 : 1;
  }

  delay(30);
}
