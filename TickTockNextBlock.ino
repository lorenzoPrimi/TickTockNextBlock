/*********
  Tick Tock Next Block
  Lorenzo Primiterra
  TODO:
  - Covert timestamp in "X min ago"
  - Add average fee in sats
  - Better UI
  - Cleaner code
  *********/

#include <Wire.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>
#include <SSD1306Wire.h>
#include <OLEDDisplayUi.h>
#include "WeatherStationImages.h"
#include <ArduinoJson.h>
#include <TimeLib.h>

const int I2C_DISPLAY_ADDRESS = 0x3c;
const int SDA_PIN = D3;
const int SDC_PIN = D4;

// WIFI
const char* WIFI_SSID = "Wi-Fi Network";
const char* WIFI_PWD = "password";

unsigned long messageInterval = 5000;
bool connected = false;

// Initialize the oled display for address 0x3c
// sda-pin=14 and sdc-pin=12

SSD1306Wire display(I2C_DISPLAY_ADDRESS, SDA_PIN, SDC_PIN);
OLEDDisplayUi ui(&display);

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println();

  // initialize dispaly
  display.init();
  display.clear();
  display.display();

  //display.flipScreenVertically();
  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.setContrast(255);

  WiFi.begin(WIFI_SSID, WIFI_PWD);

  int counter = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    display.clear();
    display.drawString(64, 10, "Connecting to WiFi");
    display.drawXbm(46, 30, 8, 8, counter % 3 == 0 ? activeSymbole : inactiveSymbole);
    display.drawXbm(60, 30, 8, 8, counter % 3 == 1 ? activeSymbole : inactiveSymbole);
    display.drawXbm(74, 30, 8, 8, counter % 3 == 2 ? activeSymbole : inactiveSymbole);
    display.display();

    counter++;
  }

  ui.setTargetFPS(30);

  ui.setActiveSymbol(activeSymbole);
  ui.setInactiveSymbol(inactiveSymbole);

  ui.init();

  display.drawString(64, 15, "Updating blocks...");
  display.display();

  //ui.init();

  //Serial.println("");

  //updateData(&display);
}

unsigned long lastUpdate = millis();

//From https://github.com/esp8266/Arduino/blob/master/libraries/ESP8266HTTPClient/examples/BasicHttpsClient/BasicHttpsClient.ino
void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    std::unique_ptr<BearSSL::WiFiClientSecure> client(new BearSSL::WiFiClientSecure);
    //client.setFingerprint("A3 99 D8 37 E4 D0 51 E2 4D 25 38 9D 59 A3 A8 DD B4 EF CA C5");
    client->setInsecure();
    HTTPClient http;  //Object of class HTTPClient
    http.begin(*client, "https://mempool.space/api/v1/blocks");
    int httpCode = http.GET();

    if (httpCode > 0) {
      const size_t bufferSize = JSON_OBJECT_SIZE(2) + JSON_OBJECT_SIZE(3) + JSON_OBJECT_SIZE(5) + JSON_OBJECT_SIZE(8) + 370;
      DynamicJsonDocument doc(1024);
      StaticJsonDocument<64> filter;
      filter[0]["height"] = true;
      filter[0]["tx_count"] = true;
      filter[0]["timestamp"] = true;
      filter[0]["extras"]["medianFee"] = true;

      //Serial.println(http.getString());

      auto error = deserializeJson(doc, http.getString(), DeserializationOption::Filter(filter));
      if (error) {
        Serial.print(F("deserializeJson() failed with code "));
        Serial.println(error.c_str());
        return;
      }

      JsonArray root = doc.as<JsonArray>();
      updateData(&display, root);
    }
    http.end();  //Close connection
  }
  delay(60000);
}

void updateData(OLEDDisplay* display, JsonArray root) {
  //delay(2000);
  int height = root[0]["height"];
  Serial.print("height:");
  Serial.println(height);
  int tx_count = root[0]["tx_count"];
  //Serial.print("tx_count:");
  //Serial.println(tx_count);
  int timestamp = root[0]["timestamp"];
  //Serial.print("timestamp:");
  //Serial.println(timestamp);
  int medianFee = root[0]["extras"]["medianFee"];
  //Serial.print("medianFee:");
  //Serial.println(medianFee);

  display->clear();
  display->setFont(ArialMT_Plain_16);
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->drawString(5, 5, "Block: " + String(height));

  display->setFont(ArialMT_Plain_10);
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->drawString(5, 30, "TX: " + String(tx_count));

  display->setFont(ArialMT_Plain_10);
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->drawString(5, 40, "Timestamp: " + String(timestamp));

  /*
  time_t t = getTime();
  time_t t_block = timestamp;
  Serial.println(t);
  Serial.println(t_block);

  //Serial.println(hour(t));
  int diff = difftime(t_block, t);
  Serial.println(diff / 60);
  Serial.println("Minutes ago");
  display->drawString(5, 45, hour(t) + " minutes ago"));
  */

  // Display static text
  //display->println("height:" + height);
  //display->println("tx_count:" + tx_count);
  //display->println("timestamp:" + timestamp);
  //display->println("medianFee:" + medianFee);

  display->display();
}
