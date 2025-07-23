#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>

#define HTTP_LED_PIN 14      // Πρώτο LED: HTTP
#define WEBHOOK_LED_PIN 13   // Δεύτερο LED: TTN webhook

// TODO: Enter your own WiFi credentials below:
const char* ssid = "YOUR_WIFI_SSID";      // <-- Replace with your WiFi SSID
const char* password = "YOUR_WIFI_PASSWORD";  // <-- Replace with your WiFi password

WebServer server(80);

void handleOn() {
  digitalWrite(HTTP_LED_PIN, HIGH);
  server.send(200, "text/plain", "HTTP LED ON");
  Serial.println("HTTP LED ON (via HTTP)");
}
void handleOff() {
  digitalWrite(HTTP_LED_PIN, LOW);
  server.send(200, "text/plain", "HTTP LED OFF");
  Serial.println("HTTP LED OFF (via HTTP)");
}

void handleWebhook() {
  if (server.hasArg("plain")) {
    String body = server.arg("plain");
    StaticJsonDocument<2048> doc;
    DeserializationError error = deserializeJson(doc, body);
    if (error) {
      Serial.println("Invalid JSON received (webhook).");
      server.send(400, "text/plain", "Invalid JSON");
      return;
    }

    if (doc["uplink_message"].isNull() ||
        doc["uplink_message"]["decoded_payload"].isNull() ||
        !doc["uplink_message"]["decoded_payload"].containsKey("mq")) {
      Serial.println("Webhook: 'mq' not found in JSON payload.");
      server.send(400, "text/plain", "mq not found");
      return;
    }

    int mq = doc["uplink_message"]["decoded_payload"]["mq"];
    Serial.print("Webhook: mq = "); Serial.println(mq);

    if (mq > 400) {
      digitalWrite(WEBHOOK_LED_PIN, HIGH);
      Serial.println("WEBHOOK LED ON (mq > 400)");
    } else {
      digitalWrite(WEBHOOK_LED_PIN, LOW);
      Serial.println("WEBHOOK LED OFF (mq <= 400)");
    }
    server.send(200, "text/plain", "OK");
  } else {
    Serial.println("Webhook: No body!");
    server.send(400, "text/plain", "No body");
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(HTTP_LED_PIN, OUTPUT);
  pinMode(WEBHOOK_LED_PIN, OUTPUT);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  Serial.print("\nESP2 IP: "); Serial.println(WiFi.localIP());

  server.on("/on", handleOn);
  server.on("/off", handleOff);
  server.on("/data", HTTP_POST, handleWebhook);
  server.begin();
  Serial.println("ESP2 ready (HTTP LED + WEBHOOK LED for TTN uplink)");
}

void loop() {
  server.handleClient();
}
