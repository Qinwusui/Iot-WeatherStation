#include <msgSender.h>

void sendGroupMsg(int64_t groupId, String msg)
{
}
void sendGroupAtMsg(WebSocketsClient *client, int64_t qq, int64_t groupId, String msg)
{
    DynamicJsonDocument json(200 + msg.length());
    json["action"] = "send_group_msg";
    json["params"]["group_id"] = groupId;
    json["params"]["message"][0]["type"] = "at";
    json["params"]["message"][0]["data"]["qq"] = qq;
    json["params"]["message"][1]["type"] = "text";
    json["params"]["message"][1]["data"]["text"] = msg;
    String js="";
    serializeJson(json, js);
    client->sendTXT(js);
}
void sendPrivateMsg(WebSocketsClient *client, int64_t qq, String msg)
{
}