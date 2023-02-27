#include <Arduino.h>
#include <ArduinoJson.h>
#include <WebSocketsClient.h>
void sendGroupMsg(int64_t groupId, String msg);

void sendGroupAtMsg(WebSocketsClient *client, int64_t qq, int64_t groupId, String msg);

void sendPrivateMsg(WebSocketsClient *client, int64_t qq, String msg);