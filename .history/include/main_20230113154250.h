#include <Arduino.h>
#include <ArduinoJson.h>
#include <Adafruit_GFX.h>
#define _TASK_INLINE
#define _TASK_STATUS_REQUEST
#define _TASK_STD_FUNCTION
#include <TaskScheduler.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>

#include <WiFiClient.h>
#include <WebSocketsServer.h>
#include <WebSocketsClient.h>
#include <DHT.h>
#include <ESP8266WebServer.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <FS.h>
#include <LittleFS.h>

#include <TFT_eSPI.h>
#include <data.h>
#include <logo.h>
#include <TaskScheduler.h>
void handleRoot();
void initTFT();
void tft_Print_Bottom(String s);
void tft_Print_Bottom_Right(String s);
void doSth(uint8_t *s);
void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length);
void initLitteFs();
String getWifiConfig();
String readPwd();
String readSSID();
void initServer();
void handleConfigWifi();
void initWifi();
void initWebSocketServer();
void ttimeUpdate();
void initWeather();
String readLoc();
void handleLocation();
void handleResetLocation();
void getPublicIp();
void getLonAndLat(String ip);
void tft_Print_Bottom_Left(String s);
void tft_Clear_Bottom();
void connectionCheck();
void displayWeather();
void initTimeClient();
stru
// int ungzip(char *source, int len, char *des);