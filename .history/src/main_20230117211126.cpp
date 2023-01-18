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
// 初始化WiFi配网
ESP8266WiFiMulti WiFiMulti;
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
Task tWeatherDisplayTask(&displayWeather, &sc);
// 定义一个WiFi连接任务
Task tWifiConnectTask(&initWifi, &sc);
// 定义一个文件系统初始化任务
Task tInitLittleFsTask(&initLitteFs, &sc);

// 屏幕驱动
TFT_eSPI tft = TFT_eSPI();

String msg = "";

String WeatherKey = "701990355eae469d87ca79642be368fc";
String IP_Key = "839945f06ffc5e";
weather wa;
String wifiFile = "/wifi/wifiConfig.ini";
String locFile = "/loc/location.ini";

void setup()
{
    Serial.begin(115200);
    dht.begin();
    initTFT();
    initServer();

    Serial.println("执行多任务");
    // 初始化存储
    tInitLittleFsTask.enable();

    // 初始化Wifi连接
    tWifiConnectTask.waitFor(tInitLittleFsTask.getInternalStatusRequest(), TASK_SECOND, -1);
    tWeatherUpdateTask.waitForDelayed(tWifiConnectTask.getInternalStatusRequest(), TASK_IMMEDIATE, -1);
    // 初始化时间更新
    tTimeInitTask.waitFor(tWifiConnectTask.getInternalStatusRequest(), TASK_IMMEDIATE, TASK_FOREVER);
}
// 循环函数
void loop()
{
    sc.execute();
}
// 初始化网络时间函数
void initTimeClient()
{
    if (!WiFi.isConnected())
    {
        Serial.println("WiFi未连接");
        tTimeInitTask.getInternalStatusRequest()->signal(-1);
        tTimeInitTask.restartDelayed(5000);
    }

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
    tft.fillRect(0, 220, tft.width(), 20, tft.color565(67, 77, 125));
    tft.setTextColor(tft.color565(240, 238, 239), tft.color565(67, 77, 125));
    tft.drawXBitmap(0, 0, Logo, 32, 32, TFT_WHITE);
    tft_Print_Bottom("Welcome!");
    delay(2000);
}
// 在屏幕右下角输出
void tft_Print_Bottom_Right(String s)
{
    int text_width = tft.textWidth(s);
    int x = tft.width() - text_width;
    tft.fillRect(0, 220, tft.width(), 20, tft.color565(67, 77, 125));

    tft.setCursor(x, 224);
    tft.println(s);
}
void tft_Print_Bottom_Left(String s)
{
    tft.setTextSize(2);
    tft.setCursor(0, 224);
    tft.fillRect(0, 220, tft.width(), 20, tft.color565(67, 77, 125));
    tft.println(s);
    tft.setTextSize(1);
}
void tft_Print_Bottom(String s)
{
    int text_width = tft.textWidth(s);
    int x = (tft.width() - text_width) / 2;
    tft.fillRect(0, 220, tft.width(), 20, tft.color565(67, 77, 125));
    tft.setCursor(x, 227);
    tft.println(s);
}
void tft_Clear_Bottom()
{
    tft.fillRect(0, 220, tft.width(), 20, tft.color565(67, 77, 125));
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
    bool runOk = WiFi.softAP("NodeMCU-12E", "qinsansui233", 10, 0, 8);
    if (!runOk)
    {
        return;
    }
    server.on("/", HTTP_GET, handleRoot);
    server.on("/config/wifi", HTTP_GET, handleConfigWifi);
    server.on("/config/wifi/list", HTTP_GET, handleWifiList);
    server.on("/config/location", HTTP_GET, handleSetLocation);

    server.begin();
}
void handleWifiList(AsyncWebServerRequest *req)
{
    onComplete = std::bind(onScanComplete, req, std::placeholders::_1);
    Serial.println("开始扫描");

    WiFi.scanNetworksAsync(onComplete);
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
        req->send(200, "application/json", s);
        tWifiConnectTask.disable();
        tWifiConnectTask.enable();
    }
}

void handleRoot(AsyncWebServerRequest *req)
{
    DynamicJsonDocument json(100);
    String s;
    json["code"] = 0;
    json["msg"] = "Wellcome！";
    json["routers"]["wifi"] = "/config/wifi";
    json["routers"]["location"] = "/config/location";

    serializeJsonPretty(json, s);
    req->send(200, "application/json", s);
}

// 初始化Wifi
void initWifi()
{

    Serial.println("初始化WiFi");
    tft_Print_Bottom("Init WiFi...");
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

    WiFi.begin(ssid, pwd);
    
    
    yield();
    Serial.println("尝试连接WiFi");
    tWifiConnectTask.yield(&connectionCheck);
}
void onWiFiEventStationModeConnected(WiFiEventStationModeConnected s)
{
    Serial.println(s.ssid);
}
void onSoftAPModeStationConnected(WiFiEventSoftAPModeStationConnected s)
{
    String mac = "";
    for (uint8_t i = 0; i < 6; i++)
    {
        mac += ":" + s.mac[i];
    }

    Serial.println(mac);
}

void connectionCheck()
{
    if (WiFi.status() == WL_CONNECTED)
    {
        Serial.println("连接成功");
        tft_Print_Bottom(WiFi.SSID() + " Connected! IP:" + WiFi.localIP().toString());
        tWifiConnectTask.delay(1000);
        tft_Clear_Bottom();
        tWifiConnectTask.getStatusRequest()->signalComplete();
        tWifiConnectTask.disable();
    }
    else // 没有连接成功，重试
    {

        if (tWifiConnectTask.getRunCounter() % 3 == 0) // 每5秒重试一次
        {
            Serial.println("重试中..");
            WiFi.begin(readSSID(), readPwd());
            yield();
        }

        if (tWifiConnectTask.getRunCounter() == 60) // 当到了20秒后，不再重试
        {
            Serial.println("重试了20次了");
            tft_Print_Bottom("Connect" + readSSID() + "Failed");
            WiFi.disconnect(true);
            tWifiConnectTask.getInternalStatusRequest()->signal(-1);
            tWifiConnectTask.disable();
        }
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
        tft.println("Temp:" + wa.tmp);
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
            Serial.println(payload);
            tWea
            tWeatherUpdateTask.getInternalStatusRequest()->signalComplete();
            tWeatherUpdateTask.setInterval(TASK_HOUR);
            tWeatherDisplayTask.enable();
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