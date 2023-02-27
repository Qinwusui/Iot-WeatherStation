#include <Arduino.h>
#include <pgmspace.h>
#include <ArduinoJson.h>
#include <Adafruit_GFX.h>
#define _TASK_INLINE
#define _TASK_STATUS_REQUEST
#define _TASK_STD_FUNCTION
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>
#include <ESPAsyncWebServer.h>
#include <ESPAsyncTCP.h>
#include <WebSocketsClient.h>
#include <WiFiClient.h>
#include <DHT.h>

#include <NTPClient.h>
#include <WiFiUdp.h>
#include <FS.h>
#include <LittleFS.h>

#include <TFT_eSPI.h>
#include <data.h>
#include <logo.h>
#include <TaskScheduler.h>
// 自定义头文件
#include <msghandler.h>
void handleRoot(AsyncWebServerRequest *req);
void initTFT();
void tft_Print_Bottom(String s);
void tft_Print_Bottom_Right(String s);
void initLitteFs();
String getWifiConfig();
String readPwd();
String readSSID();
void initServer();
void handleConfigWifi(AsyncWebServerRequest *req);
void initWifi();
void ttimeUpdate();
void initWeather();
String readLoc();
void handleSetLocation(AsyncWebServerRequest *req);

void tft_Print_Bottom_Left(String s);
void tft_Clear_Bottom();
void connectionCheck();
void displayWeather();
void initTimeClient();
void handleWifiList(AsyncWebServerRequest *req);

std::function<void(int)> onCompleteBYWeb;
void onScanComplete(AsyncWebServerRequest *req, int n);

void writeLoc(String lon, String lat);
scu *rssiToString(int8_t rssi);
void showWifiIcon(const uint8_t *img);
void wifiConnecting();
void disConnectWifi();
void showWifiInfoToTop();
void showMsgToTop(String str);
void tft_Clear_Top();
void initWsClient();
void wsClientEvent(WStype_t type, uint8_t *payload, size_t length);
void wsLoop();
void time2Logo();
void timeUp();
// int ungzip(char *source, int len, char *des);