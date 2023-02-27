#include <msgSender.h>
#include <ESP8266WiFi.h>

void groupMsgHandler(WebSocketsClient *client, int64_t groupId, int64_t senderId, String msg, String rawMsg);
void privateMsgHandler(int64_t sender, String msg, String rawMsg);

void onWifiScanComplete(int n, WebSocketsClient *client, int64_t senderId, int64_t groupId);
