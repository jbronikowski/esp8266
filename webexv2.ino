#include <FS.h>                   //this needs to be first, or it all crashes and burns...

#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino

//needed for library
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager

#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h> 
#include <WiFiClientSecureBearSSL.h>

// Fingerprint for demo URL, expires on June 2, 2021, needs to be updated well before this date
const char fingerprint[] PROGMEM = "4C BA 93 17 2D F3 CA FA 67 1A 10 09 61 7A D9 68 45 93 1A BF";


const int RED_PIN = 0;
const int GREEN_PIN = 2;
const int BLUE_PIN = 5;

char apikey[250];
char email[250];

//flag for saving data
bool shouldSaveConfig = false;

//callback notifying us of the need to save config
void saveConfigCallback () {
  Serial.println("Should save config");
  shouldSaveConfig = true;
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.println();

  pinMode(RED_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(BLUE_PIN, OUTPUT);

  digitalWrite(RED_PIN, HIGH);
  digitalWrite(GREEN_PIN, HIGH);
  digitalWrite(BLUE_PIN, HIGH);

  //clean FS, for testing
  //SPIFFS.format();

  //read configuration from FS json
  Serial.println("mounting FS...");

  if (SPIFFS.begin()) {
    Serial.println("mounted file system");
    if (SPIFFS.exists("/config.json")) {
      //file exists, reading and loading
      Serial.println("reading config file");
      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile) {
        Serial.println("opened config file");
        size_t size = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);
        DynamicJsonDocument json(1024);
        DeserializationError error = deserializeJson(json, buf.get());
        if (!error) {
          Serial.println("\nparsed json");

          strcpy(apikey, json["apikey"]);
          strcpy(email, json["email"]);

        } else {
          Serial.println("failed to load json config");
        }
        configFile.close();
      }
    }
  } else {
    Serial.println("failed to mount FS");
  }
  //end read



  // The extra parameters to be configured (can be either global or just in the setup)
  // After connecting, parameter.getValue() will get you the configured value
  // id/name placeholder/prompt default length
  WiFiManagerParameter custom_apikey("apikey", "Token Here", apikey, 250);
  WiFiManagerParameter custom_email("email", "Email Here", email, 250);
  //WiFiManager
  //Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;

  //set config save notify callback
  wifiManager.setSaveConfigCallback(saveConfigCallback);

  //exit after config instead of connecting
  wifiManager.setBreakAfterConfig(true);

  wifiManager.addParameter(&custom_apikey);
  wifiManager.addParameter(&custom_email);

  //reset settings - for testing
  //wifiManager.resetSettings();


  //tries to connect to last known settings
  //if it does not connect it starts an access point with the specified name
  //here  "AutoConnectAP" with password "password"
  //and goes into a blocking loop awaiting configuration
  if (!wifiManager.autoConnect("AutoConnectAP", "password")) {
    Serial.println("failed to connect, we should reset as see if it connects");
    delay(3000);
    ESP.reset();
    delay(5000);
  }

  //if you get here you have connected to the WiFi
  Serial.println("connected...yeey :)");
  //read updated parameters
  strcpy(apikey, custom_apikey.getValue());
  strcpy(email, custom_email.getValue());

  //save the custom parameters to FS
  if (shouldSaveConfig) {
    Serial.println("saving config");
    DynamicJsonDocument json(1024);
    json["apikey"] = apikey;
    json["email"] = email;

    File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile) {
      Serial.println("failed to open config file for writing");
    }
     // Serialize JSON to file
    if (serializeJson(json, configFile) == 0) {
      Serial.println(F("Failed to write to file"));
    }

    configFile.close();
    //end save
  }

  Serial.println("local ip");
  Serial.println(WiFi.localIP());
}

void loop() {
  // wait for WiFi connection
  if ((WiFi.localIP())) {

    Serial.println("===============");
    Serial.println(apikey);
    Serial.println(email);

    std::unique_ptr<BearSSL::WiFiClientSecure>client(new BearSSL::WiFiClientSecure);

    client->setFingerprint(fingerprint);

    HTTPClient https;

    char url[255];
    strcpy(url, "https://api.ciscospark.com/v1/people?email=");
    strcat(url, email);

    char token[1024];
    strcpy(token, "Bearer ");
    strcat(token, apikey);
    
    Serial.print("[HTTPS] begin...\n");
    if (https.begin(*client, url)) {  // HTTPS
      https.addHeader("Content-Type", "application/json");
      https.addHeader("Authorization", token); 

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
            digitalWrite(GREEN_PIN, HIGH);
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

  Serial.println("Wait 2s before next round...");
  delay(2000);
}
