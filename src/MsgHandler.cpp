#include <msghandler.h>
#include <functional>
using namespace std;

void groupMsgHandler(WebSocketsClient *client, int64_t groupId, int64_t senderId, String msg, String rawMsg)
{
    switch (groupId)
    {
    case 329972361:
        // 泰拉
        {
        }
        break;
    case 637081363:
        // 分享
        {

            if (msg == "/esp8266")
            {
                String message = "";
                message += "ESP8266 Status\n";
                message += "WiFi:" + WiFi.SSID() + " Connected" + WiFi.isConnected() + "\n";
                sendGroupAtMsg(client, senderId, groupId, message + "\n---By Esp8266");
            }
            if (msg == "/wifiList")
            {
                std::function<void(int)> onCompleteBYBot = bind(onWifiScanComplete, placeholders::_1, client, senderId, groupId);
                Serial.println("开始扫描");
                WiFi.scanNetworksAsync(onCompleteBYBot);
            }
           
        }
        break;

    default:
        break;
    }
}
void onWifiScanComplete(int n, WebSocketsClient *client, int64_t senderId, int64_t groupId)
{
    String list = "\n";

    for (int8_t i = 0; i < n; i++)
    {
        String ssid = WiFi.SSID(i);
        int32_t channel = WiFi.channel(i);
        int32_t rssi = WiFi.RSSI(i);
        uint8_t type = WiFi.encryptionType(i);
        if (i != 0 || i != n - 1)
        {
            list += "\n";
        }
        list += "SSID:" + ssid;
    }
    sendGroupAtMsg(client, senderId, groupId, list);
}
void privateMsgHandler(int64_t sender, String msg, String rawMsg)
{
}
