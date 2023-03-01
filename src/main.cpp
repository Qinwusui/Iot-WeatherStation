/**
 * @file main.cpp
 * @author Qinwusui
 * @brief
 * @version 0.1
 * @date 2022-11-27
 *
 * @copyright Copyright (c) 2022
 *
 */
#include <main.h>
// WebSocket客户端
WebSocketsClient wsClient;
// 初始化温度传感器
DHT dht(15, DHT11);
// 初始化服务器
AsyncWebServer server(80);
// 获取网络时间
// 初始化UDP
WiFiUDP ntpUDP;
// 初始化时间管理器
NTPClient timeClient(ntpUDP, "ntp.ntsc.ac.cn", 28800);
// 定义调度器
Scheduler sc;
// 定义一个天气更新任务
Task tWeatherUpdateTask(&initWeather, &sc);
// 定义一个时间更新任务
Task tTimeUpdateTask(&ttimeUpdate, &sc);
// 定义一个时间初始化任务
Task tTimeInitTask(&initTimeClient, &sc);
// 定义一个天气展示任务
Task tWeatherDisplayTask(TASK_IMMEDIATE, TASK_FOREVER, &displayWeather, &sc);
// 定义一个WiFi连接任务
Task tWifiConnectTask(TASK_SECOND, TASK_FOREVER, &initWifi, &sc);
// 定义一个文件系统初始化任务
Task tInitLittleFsTask(&initLitteFs, &sc);

// 定义一个WiFi连接中动画任务
Task tWifiConnectingAnimationTask(TASK_SECOND, TASK_FOREVER, &wifiConnecting, &sc);
// 定义一个展示ssid的任务
Task tShowWifiInfoToTopTask(TASK_IMMEDIATE, TASK_FOREVER, &showWifiInfoToTop, &sc, true);
// 定义一个初始化Websocket客户端任务
Task tInitWsClientTask(TASK_IMMEDIATE, TASK_ONCE, &initWsClient, &sc);
// 定义一个WebSocket轮询任务
Task tWsClientPollTask(TASK_IMMEDIATE, TASK_FOREVER, &wsLoop, &sc);
// 定义一个展示LogoGIF的任务
Task tShowLogoTask(TASK_IMMEDIATE, TASK_FOREVER, &time2Logo, &sc);

// 定义一个计时任务
Task tTickerTask(TASK_SECOND, TASK_FOREVER, &timeCount, &sc);
int timeStamp = 0;

// 屏幕驱动
TFT_eSPI tft = TFT_eSPI();

String msg = "";

String WeatherKey = "701990355eae469d87ca79642be368fc";
String IP_Key = "839945f06ffc5e";
weather wa;
String wifiFile = "/wifi/wifiConfig.ini";
String locFile = "/loc/location.ini";
uint16_t bgColor = tft.color565(245, 189, 188);

void setup()
{
    Serial.begin(115200);
    dht.begin();
    initTFT();

    initServer();

    Serial.println("执行多任务");
    // 初始化存储
    tInitLittleFsTask.enable();
    tTickerTask.enable();
    tShowLogoTask.enable();
    // 初始化Wifi连接
    tWifiConnectTask.waitFor(tInitLittleFsTask.getInternalStatusRequest(), TASK_SECOND, -1);
    tInitWsClientTask.waitFor(tWifiConnectTask.getInternalStatusRequest(), TASK_IMMEDIATE, TASK_ONCE);
    tWsClientPollTask.waitFor(tInitWsClientTask.getInternalStatusRequest(), TASK_IMMEDIATE, TASK_FOREVER);
    tWeatherUpdateTask.waitForDelayed(tWifiConnectTask.getInternalStatusRequest(), TASK_IMMEDIATE, -1);
    // 初始化时间更新
    tTimeInitTask.waitFor(tWifiConnectTask.getInternalStatusRequest(), TASK_IMMEDIATE, TASK_FOREVER);
}
// 循环函数
void loop()
{
    sc.execute();
}
void initWsClient()
{
    Serial.println("初始化Websocket客户端");
    wsClient.begin("192.168.123.100", 8080, "/");
    if (wsClient.isConnected())
    {
        Serial.println("已连接Ws服务器");
    }
    wsClient.onEvent(wsClientEvent);
    tInitWsClientTask.getInternalStatusRequest()->signalComplete();
}
void wsLoop()
{
    wsClient.loop();
}
void wsClientEvent(WStype_t type, uint8_t *payload, size_t length)
{
    String text = String((char *)payload);

    switch (type)
    {
    case WStype_TEXT:
    {
        DynamicJsonDocument json(length * 2);
        deserializeJson(json, text);
        String postType = json["post_type"].as<String>();
        if (postType == "message")
        {
            String messageType = json["message_type"].as<String>();
            if (messageType == "group")
            {
                // 群聊消息
                int64_t groupId = json["group_id"].as<int64_t>();
                int64_t senderId = json["sender"]["user_id"].as<int64_t>();
                String msg = json["message"].as<String>();
                String rawMsg = json["raw_message"].as<String>();
                groupMsgHandler(&wsClient, groupId, senderId, msg, rawMsg);
            }
            if (messageType == "private")
            {
                // 私聊消息
            }
        }
    }

    break;

    default:
        break;
    }
}

// 初始化网络时间函数
void initTimeClient()
{
    Serial.println("初始化时间");
    timeClient.begin();

    tTimeInitTask.getInternalStatusRequest()->signalComplete();
    tTimeUpdateTask.setInterval(1000);
    tTimeUpdateTask.setIterations(TASK_FOREVER);
    tTimeUpdateTask.enable();
    tTimeInitTask.disable();
}
// 初始化屏幕
void initTFT()
{
    tft.begin();
    tft.setRotation(1);

    tft.fillScreen(TFT_BLACK);

    tft.fillRect(0, 220, tft.width(), 20, bgColor);
    tft.fillRect(0, 0, tft.width(), 20, bgColor);

    tft.setTextColor(tft.color565(32, 32, 32), bgColor);

    tft_Print_Bottom("Welcome!");
    delay(2000);
}
void timeCount()
{
    if (timeStamp == 100000)
    {
        timeStamp = 0;
    }
    timeStamp++;
}
void time2Logo()
{
    tft.fillRect(0, 0, 24, 20, bgColor);
    if (timeStamp % 2 == 0)
    {
        tft.drawXBitmap(0, 0, Logo1, 24, 24, TFT_BLACK);
    }
    else
    {
        tft.drawXBitmap(0, 0, Logo2, 24, 24, TFT_BLACK);
    }
}
// 在屏幕右下角输出
void tft_Print_Bottom_Right(String s)
{
    int text_width = tft.textWidth(s);
    int x = tft.width() - text_width;
    tft.fillRect(0, 220, tft.width(), 20, bgColor);
    tft.setCursor(x, 224);
    tft.println(s);
}
void tft_Print_Bottom_Left(String s)
{
    tft.setTextSize(2);
    tft.setCursor(0, 224);
    tft.fillRect(0, 220, tft.width(), 20, bgColor);
    tft.println(s);
    tft.setTextSize(1);
}
void tft_Print_Bottom(String s)
{
    int text_width = tft.textWidth(s);
    int x = (tft.width() - text_width) / 2;
    tft.fillRect(0, 220, tft.width(), 20, bgColor);
    tft.setCursor(x, 227);
    tft.println(s);
}
void tft_Clear_Top()
{
}
void tft_Clear_Bottom()
{
    tft.fillRect(0, 220, tft.width(), 20, bgColor);
}

void initLitteFs()
{
    if (tInitLittleFsTask.isFirstIteration())
    {
        Serial.println("首次执行初始Fs");
    }

    Serial.println("初始化FS");
    tft_Print_Bottom("Loading FileSys...");

    if (LittleFS.begin())
    {
        if (!LittleFS.exists(wifiFile))
        {
            // 不存在WiFi文件
            tInitLittleFsTask.getInternalStatusRequest()->signal(-1);
            tft_Print_Bottom("Have Not WIFI Config");
        }
        if (!LittleFS.exists(locFile))
        {
            tInitLittleFsTask.getInternalStatusRequest()->signal(-1);
            tft_Print_Bottom("Have Not Location Config");
        }

        tft_Print_Bottom("Initiated FileSys...");
        tInitLittleFsTask.getInternalStatusRequest()->signalComplete();
        tInitLittleFsTask.disable();
        Serial.println("LittleFs初始化完成");
    }
    else
    {
        tInitLittleFsTask.getInternalStatusRequest()->signal(-1);
        tInitLittleFsTask.disable();

        tft_Print_Bottom("Initiating FileSys faild!");
    }
}
String getWifiConfig()
{
    String wifi("");

    File f = LittleFS.open(wifiFile, "r");
    for (size_t i = 0; i < f.size(); i++)
    {
        wifi += f.readString();
    }
    f.close();
    return wifi;
}
String readPwd()
{
    String wifi = getWifiConfig();
    int index = wifi.indexOf("||");
    String s2 = wifi.substring(index + 2, wifi.length());
    return s2;
}
String readSSID()
{
    String wifi = getWifiConfig();
    int index = wifi.indexOf("||");
    String s1 = wifi.substring(0, index).c_str();
    return s1;
}

// 通过Web进行配网
void initServer()
{
    tft_Print_Bottom("Create WebServer...");
    WiFi.mode(WIFI_AP_STA);
    bool runOk = WiFi.softAP("MERCURY", "qinsansui233", 10, 0, 8);
    if (!runOk)
    {
        return;
    }
    server.on("/", HTTP_GET, handleRoot);
    server.on("/wifi", HTTP_GET, handleConfigWifi);
    server.on("/list", HTTP_GET, handleWifiList);
    server.on("/location", HTTP_GET, handleSetLocation);

    server.begin();
}
void handleWifiList(AsyncWebServerRequest *req)
{
    onCompleteBYWeb = std::bind(onScanComplete, req, std::placeholders::_1);
    Serial.println("开始扫描");
    WiFi.scanNetworksAsync(onCompleteBYWeb);
}
void onScanComplete(AsyncWebServerRequest *req, int n)
{
    DynamicJsonDocument json(80 * n + 1024);
    json["code"] = 0;
    json["msg"] = "获取成功";

    for (int8_t i = 0; i < n; i++)
    {
        String ssid = WiFi.SSID(i);
        int32_t channel = WiFi.channel(i);
        int32_t rssi = WiFi.RSSI(i);
        uint8_t type = WiFi.encryptionType(i);
        json["list"][i]["ssid"] = ssid;
        json["list"][i]["channel"] = channel;
        json["list"][i]["rssi"] = rssi;
        json["list"][i]["type"] = type;
        Serial.println(ssid);
    }
    String s;
    serializeJsonPretty(json, s);
    req->send(200, "application/json", s);
}
void handleSetLocation(AsyncWebServerRequest *req)
{
    String lon = req->arg("lon");
    String lat = req->arg("lat");
    if (lon != "" && lat != "")
    {
        tft_Print_Bottom("Setting Location...");
        writeLoc(lon, lat);

        DynamicJsonDocument json(60);
        json["code"] = 0;
        json["msg"] = "设置成功";
        String s;
        serializeJsonPretty(json, s);
        req->send(200, "application/json", s);
        tft_Print_Bottom("Set Successfully");

        tWeatherUpdateTask.disable();
        tft_Clear_Bottom();
        tWeatherUpdateTask.setInterval(5000);
        tWeatherUpdateTask.enable();
    }
}
void handleConfigWifi(AsyncWebServerRequest *req)
{
    String ssid = req->arg("ssid");
    String pwd = req->arg("pwd");
    ssid.trim();
    pwd.trim();
    if (ssid == "" || pwd.length() < 8)
    {
        DynamicJsonDocument json(100);
        json["code"] = -1;
        json["msg"] = "你可能没有填写WiFi名称或者WiFi密码";
        String s;
        serializeJsonPretty(json, s);
        req->send(200, "application/json", s);
    }
    else
    {
        String wifi = ssid + "||" + pwd;
        File f = LittleFS.open(wifiFile, "w");
        f.print(wifi);
        f.close();
        DynamicJsonDocument json(100);
        json["code"] = 0;
        json["msg"] = "提交成功，已经将配置文件存入文件，即将开始连接WiFi";
        String s;
        serializeJsonPretty(json, s);
        disConnectWifi();
        req->send(200, "application/json", s);
    }
}
void disConnectWifi()
{
    WiFi.disconnect();
    tWifiConnectingAnimationTask.enable();
    tWifiConnectTask.disable();
    tWeatherDisplayTask.disable();
    tWeatherUpdateTask.disable();
    tWifiConnectTask.restart();
}

void handleRoot(AsyncWebServerRequest *req)
{
    DynamicJsonDocument json(100);
    String s;
    json["code"] = 0;
    json["msg"] = "Wellcome！";
    json["routers"]["wifi"] = "/wifi";
    json["routers"]["location"] = "/location";

    serializeJsonPretty(json, s);
    req->send(200, "application/json", s);
}

// 初始化Wifi
void initWifi()
{

    Serial.println("初始化WiFi");
    tft_Print_Bottom("Init WiFi...");
    tWifiConnectingAnimationTask.enable();
    String ssid = readSSID();
    ssid.replace("\n", "");
    ssid.trim();
    String pwd = readPwd();
    pwd.replace("\n", "");
    pwd.trim();
    if (ssid == "" || pwd == "")
    {
        Serial.println("ssid为空");
        return;
    }
    Serial.println(ssid + pwd);

    WiFi.disconnect();
    yield();
    WiFi.begin(ssid, pwd);

    yield();
    Serial.println("尝试连接WiFi");
    tWifiConnectTask.yield(&connectionCheck);
}
void connectionCheck()
{

    if (WiFi.status() == WL_CONNECTED)
    {

        Serial.println("连接成功");
        tft_Print_Bottom(WiFi.SSID() + " Connected! IP:" + WiFi.localIP().toString());
        // 停止WiFi动画 WiFi信号展示任务 SSID

        tWifiConnectingAnimationTask.disable();

        tWeatherUpdateTask.enable();
        tft_Clear_Bottom();
        tWifiConnectTask.getStatusRequest()->signalComplete();
        tWifiConnectTask.disable();
    }
    else // 没有连接成功，重试
    {

        if (tWifiConnectTask.getRunCounter() % 5 == 0) // 每5秒重试一次
        {
            Serial.println("重试中..");
            WiFi.begin(readSSID(), readPwd());
            yield();
        }

        // if (tWifiConnectTask.getRunCounter() == 60) // 当到了20秒后，不再重试
        // {
        //     Serial.println("重试了20次了");
        //     tft_Print_Bottom("Connect" + readSSID() + "Failed");
        //     WiFi.disconnect(true);
        //     tWifiConnectTask.getInternalStatusRequest()->signal(-1);
        //     tWifiConnectTask.disable();
        // }
    }
}

void ttimeUpdate() // 1s更新一次
{
    if (tTimeUpdateTask.isFirstIteration())
    {
        Serial.println("展示时间");
    }
    timeClient.forceUpdate();

    tft.setTextSize(2);

    String time = timeClient.getFormattedTime();
    int text_width = tft.textWidth(time);
    int x = tft.width() - text_width;
    tft.setCursor(x, 224);
    tft.println(time);
    tft.setTextSize(1);
    tTimeUpdateTask.getInternalStatusRequest()->signalComplete();
}
void displayWeather()
{
    if (tWeatherDisplayTask.isFirstIteration())
    {
        Serial.println("展示天气");
    }

    if (wa.code == "200")
    {
        tft.setCursor(0, 222);
        tft.print("Temp:" + wa.tmp);
        tft.setCursor(0, 232);
        tft.print("Humi:" + wa.humidity);
    }
}

void initWeather()
{
    if (tWeatherUpdateTask.isFirstIteration())
    {
        Serial.println("初始化天气");
    }
    if (readLoc() == "")
    {
        tft_Print_Bottom("LocationFile Not Found!");
        tWeatherUpdateTask.getInternalStatusRequest()->signal(-1);
        tWeatherUpdateTask.disable();
    }
    std::unique_ptr<BearSSL::WiFiClientSecure> client(new BearSSL::WiFiClientSecure);
    client->setInsecure();
    HTTPClient https;
    https.setUserAgent("Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/109.0.0.0 Safari/537.36");
    https.addHeader("Accept", "application/json");
    https.addHeader("Accept-Encoding", "application/json");
    https.addHeader("Connection", "close");
    String url = "https://devapi.qweather.com/v7/weather/now?gzip=n&key=" +
                 WeatherKey + "&location=" + readLoc();
    if (https.begin(*client, url))
    {
        int code = https.GET();
        if (code == HTTP_CODE_OK)
        {
            String payload = https.getString();
            DynamicJsonDocument json(payload.length() * 2);
            deserializeJson(json, payload);
            JsonObject obj = json.as<JsonObject>();
            wa.code = obj["code"].as<String>();
            wa.tmp = obj["now"]["temp"].as<String>();
            wa.humidity = obj["now"]["humidity"].as<String>();
            wa.text = obj["now"]["text"].as<String>();
            Serial.println("获取到天气信息");
            tWeatherDisplayTask.disable();
            tWeatherDisplayTask.enable();
            tWeatherUpdateTask.getInternalStatusRequest()->signalComplete();
            tWeatherUpdateTask.setInterval(TASK_HOUR);
        }
    }

    if (wa.code != "200")
    {
        Serial.println("获取天气信息失败");
        Serial.println(readLoc());
        tWeatherUpdateTask.setInterval(5000);
    }
}

String readLoc() // 经纬度
{
    File f = LittleFS.open(locFile, "r");
    String loc = f.readString();
    f.close();
    return loc;
}
void writeLoc(String lon, String lat)
{
    LittleFS.remove(locFile);
    lon.trim();
    lat.trim();
    File f = LittleFS.open(locFile, "w+");
    f.print(lon + "," + lat);
    f.close();
}

scu *rssiToString(int8_t rssi)
{

    if (rssi >= -80 && rssi <= -70)
    {
        return WiFi_Low;
    }
    if (rssi > -70 && rssi <= -60)
    {
        return WiFi_Medium;
    }
    if (rssi > -60 && rssi <= -50)
    {
        return WiFi_Good;
    }
    if (rssi > -50)
    {
        return WiFi_Perfect;
    }
    return WiFi_Low;
}
void showWifiIcon(const uint8_t *img)
{
    tft.fillRect(tft.width() - 18, 2, 16, 16, bgColor);
    tft.drawXBitmap(tft.width() - 18, 2, img, 16, 16, TFT_BLACK);
}
void showMsgToTop(String str)
{
    int width = tft.textWidth(str);
    tft.setCursor(tft.width() - (20 + width), 8);
    tft.print(str);
    tShowWifiInfoToTopTask.getInternalStatusRequest()->signalComplete();
}
void showWifiInfoToTop()
{

    if (tWifiConnectTask.getInternalStatusRequest()->completed())
    {
        String ssid = readSSID();
        int width = tft.textWidth("Connecting...");
        tft.fillRect(tft.width() - (width + 20), 8, width, 8, bgColor);
        showMsgToTop(ssid);
        int rssi = WiFi.RSSI();
        scu *wifi = rssiToString(rssi);
        showWifiIcon(wifi);
    }
    else
    {
        showMsgToTop("Connecting...");
    }
}

void wifiConnecting()
{
    tft.fillRect(tft.width() - 18, 2, 16, 16, bgColor);
    if (tWifiConnectingAnimationTask.getRunCounter() % 4 == 1)
    {
        tft.drawXBitmap(tft.width() - 18, 2, WiFi_Low, 16, 16, TFT_BLACK);
    }
    if (tWifiConnectingAnimationTask.getRunCounter() % 4 == 2)
    {
        tft.drawXBitmap(tft.width() - 18, 2, WiFi_Medium, 16, 16, TFT_BLACK);
    }

    if (tWifiConnectingAnimationTask.getRunCounter() % 4 == 3)
    {
        tft.drawXBitmap(tft.width() - 18, 2, WiFi_Good, 16, 16, TFT_BLACK);
    }

    if (tWifiConnectingAnimationTask.getRunCounter() % 4 == 0)
    {
        tft.drawXBitmap(tft.width() - 18, 2, WiFi_Perfect, 16, 16, TFT_BLACK);
    }
}
