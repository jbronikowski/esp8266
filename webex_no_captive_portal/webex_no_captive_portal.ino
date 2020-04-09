/**
   BasicHTTPSClient.ino

    Created on: 20.08.2018

*/

#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>

#include <ESP8266HTTPClient.h>

#include <WiFiClientSecureBearSSL.h>
// Fingerprint for demo URL, expires on June 2, 2021, needs to be updated well before this date
const char fingerprint[] PROGMEM = "4C BA 93 17 2D F3 CA FA 67 1A 10 09 61 7A D9 68 45 93 1A BF";

ESP8266WiFiMulti WiFiMulti;

const int RED_PIN = 0;
const int GREEN_PIN = 2;
const int BLUE_PIN = 5;

void setup() {

  Serial.begin(115200);
  // Serial.setDebugOutput(true);

  pinMode(RED_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(BLUE_PIN, OUTPUT);

  digitalWrite(RED_PIN, HIGH);
  digitalWrite(GREEN_PIN, HIGH);
  digitalWrite(BLUE_PIN, HIGH);

  Serial.println();
  Serial.println();
  Serial.println();

  for (uint8_t t = 4; t > 0; t--) {
    Serial.printf("[SETUP] WAIT %d...\n", t);
    Serial.flush();
    delay(1000);
  }

  WiFi.mode(WIFI_STA);
  WiFiMulti.addAP("SSID", "PASSWORD");
}

void loop() {
  // wait for WiFi connection
  if ((WiFiMulti.run() == WL_CONNECTED)) {

    std::unique_ptr<BearSSL::WiFiClientSecure>client(new BearSSL::WiFiClientSecure);

    client->setFingerprint(fingerprint);

    HTTPClient https;

    Serial.print("[HTTPS] begin...\n");
    if (https.begin(*client, "https://api.ciscospark.com/v1/people/me")) {  // HTTPS
      https.addHeader("Content-Type", "application/json");
      https.addHeader("Authorization", "Bearer TOKEN_GOES_HERE"); 

      Serial.print("[HTTPS] GET...\n");
      // start connection and send HTTP header
      int httpCode = https.GET();

      // httpCode will be negative on error
      if (httpCode > 0) {
        // HTTP header has been send and Server response header has been handled
        Serial.printf("[HTTPS] GET... code: %d\n", httpCode);

        // file found at server
        if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
          String payload = https.getString();
          Serial.println(payload);
          String im_status = "";
          if(payload.indexOf("\"status\":\"meeting\"") > 0) {
            im_status = "Meeting";
            digitalWrite(RED_PIN, LOW);
            digitalWrite(GREEN_PIN, HIGH);
            digitalWrite(BLUE_PIN, HIGH);
          }
          if(payload.indexOf("\"status\":\"presenting\"") > 0) {
            im_status = "Presenting";
            digitalWrite(RED_PIN, HIGH);
            digitalWrite(GREEN_PIN, LOW);
            digitalWrite(BLUE_PIN, LOW);
          }
          if(payload.indexOf("\"status\":\"active\"") > 0) {
            im_status = "Active";
            digitalWrite(RED_PIN, HIGH);
            digitalWrite(GREEN_PIN, LOW);
            digitalWrite(BLUE_PIN, HIGH);
          }
          if(payload.indexOf("\"status\":\"inactive\"") > 0) {
            im_status = "Inactive";
            digitalWrite(RED_PIN, HIGH);
            digitalWrite(GREEN_PIN, HIGH);
            digitalWrite(BLUE_PIN, HIGH);
          }

          Serial.println(im_status);
          
        }
      } else {
        Serial.printf("[HTTPS] GET... failed, error: %s\n", https.errorToString(httpCode).c_str());
      }

      https.end();
    } else {
      Serial.printf("[HTTPS] Unable to connect\n");
    }
  }

  Serial.println("Wait 10s before next round...");
  delay(2000);
}
