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
ESP8266WebServer server(80);
// 获取网络时间
// 初始化UDP
WiFiUDP ntpUDP;
// 初始化时间管理器
NTPClient timeClient(ntpUDP, "ntp1.aliyun.com", 60 * 60 * 8, 1000);
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
// 定义一个获取经纬的任务
Task tInitLonAndLatTask(&getPublicIp, &sc);

// 定义一个WiFi扫描任务
Task tWifiScanTask(&wifiScanner, &sc);
// 屏幕驱动
TFT_eSPI tft = TFT_eSPI();

String msg = "";
String ssid = "";
String pwd = "";
String WeatherKey = "701990355eae469d87ca79642be368fc";
String IP_Key = "839945f06ffc5e";
String loc = "";
String locId = "";
weather wa;

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
   // 初始化时间更新
   tTimeInitTask.waitFor(tWifiConnectTask.getInternalStatusRequest(), TASK_SECOND, TASK_ONCE);
   tTimeUpdateTask.waitFor(tTimeInitTask.getInternalStatusRequest(), TASK_IMMEDIATE, -1);
   // 初始化天气更新
   tWeatherUpdateTask.waitFor(tWifiConnectTask.getInternalStatusRequest(), TASK_HOUR, -1);
   tWeatherDisplayTask.waitFor(tWeatherUpdateTask.getInternalStatusRequest(), TASK_SECOND, -1);
}
void loop()
{
   server.handleClient();

   sc.execute();
}

void initTimeClient()
{
   Serial.println("初始化时间");
   timeClient.begin();
   yield();
   tTimeInitTask.getInternalStatusRequest()->signalComplete();
   tTimeInitTask.disable();
}
// 执行一次，初始化操作
String wifiFile = "/wifi/wifiConfig.ini";
String locFile = "/loc/location.ini";

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
   server.on("/", handleRoot);
   server.on("/config/wifi", handleConfigWifi);
   server.on("/config/wifi/list", handleWifiList);
   server.on("/config/location", handleResetLocation);
   server.begin();
}
void handleWifiList()
{
   tWifiScanTask.enable();
}
void wifiScanner()
{
   WiFi.scanNetworks(true);
   int n = WiFi.scanComplete();
   if (n >= 0)
   {
      Serial.printf("找到%d个WiFi\n", n);
      for (int i = 0; i < n; i++)
      {
         Serial.printf("%s:通道:%s 强度:%d 加密类型:%s", WiFi.SSID(i),
                       WiFi.channel(i),
                       WiFi.RSSI(i),
                       WiFi.encryptionType(i));

      }
   }else
   {
      Serial.println("没有找到网络")
   }
   
}
void handleResetLocation()
{
   String reset = server.arg("reset");
   if (reset == "true")
   {
      tft_Print_Bottom("Resetting Location...");
      DynamicJsonDocument json(30);
      json["code"] = 0;
      json["msg"] = "重置成功！";
      String s;
      serializeJsonPretty(json, s);
      server.send(200, "text/javascript", s);
      tInitLonAndLatTask.enable();
      tft_Print_Bottom("Reset Successfully");
      delay(1000);
      tft_Clear_Bottom();
   }
}
void handleConfigWifi()
{
   ssid = server.arg("ssid");
   pwd = server.arg("pwd");
   if (ssid == "" || pwd.length() < 8)
   {
      DynamicJsonDocument json(60);
      json["code"] = -1;
      json["msg"] = "你可能没有填写WiFi名称或者WiFi密码";
      String s;
      serializeJsonPretty(json, s);
      server.send(200, "text/javascript", s);
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
      server.send(200, "text/javascript", s);

      tWifiConnectTask.restart();
   }
}

void handleRoot()
{
   DynamicJsonDocument json(100);
   String s;
   json["code"] = 0;
   json["msg"] = "Wellcome！";
   json["routers"]["wifi"] = "/config/wifi";
   json["routers"]["location"] = "/config/location";

   serializeJsonPretty(json, s);
   server.send(200, "text/javascript", s);
}

// 初始化Wifi
void initWifi()
{

   Serial.println("初始化WiFi");
   tft_Print_Bottom("Init WiFi...");
   ssid = readSSID();
   ssid.replace("\n", "");
   ssid.trim();
   pwd = readPwd();
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

void connectionCheck()
{
   if (WiFi.status() == WL_CONNECTED)
   {
      Serial.println("连接成功");

      File f = LittleFS.open(locFile, "r");
      if (f.readString() == "0.00,0.00")
      {
         tInitLonAndLatTask.enable();
      }
      f.close();
      tft_Print_Bottom(WiFi.SSID() + " Connected! IP:" + WiFi.localIP().toString());
      tWifiConnectTask.getStatusRequest()->signalComplete();
      tWifiConnectTask.disable();
   }
   else // 没有连接成功，重试
   {

      if (tWifiConnectTask.getRunCounter() % 5 == 0) // 每5秒重试一次
      {
         Serial.println("重试中..");
         WiFi.begin(ssid, pwd);
         yield();
      }

      if (tWifiConnectTask.getRunCounter() == 30) // 当到了20秒后，不再重试
      {
         Serial.println("重试了6次了");
         tft_Print_Bottom("Connect" + ssid + "Failed");
         WiFi.disconnect(true);
         tWifiConnectTask.getInternalStatusRequest()->signal(-1);
         tWifiConnectTask.disable();
      }
   }
}
void getPublicIp()
{
   std::unique_ptr<BearSSL::WiFiClientSecure> client(new BearSSL::WiFiClientSecure);
   client->setInsecure();
   // WiFiClient client;
   HTTPClient https;
   https.setTimeout(6000);
   https.addHeader("Accept-Encoding", "none", false);
   https.addHeader("Cache-Control", "no-cache");
   https.addHeader("Connection", "keep-alive");
   String url = "https://www.ip.cn/api/index?ip=&type=0";
   if (https.begin(*client, url))
   {
      int code = 0;
      if ((code = https.GET()) == 200)
      {
         String payload = https.getString();
         DynamicJsonDocument json(payload.length() * 2);
         deserializeJson(json, payload);
         String ip = json["ip"].as<String>();
         Serial.println("获取到IP:" + ip);
         if (ip != "")
         {
            getLonAndLat(ip);
         }
         else
         {
            tft_Print_Bottom("Failed to Get IP");
         }
      }
   }
}
void getLonAndLat(String ip)
{
   WiFiClient client;
   HTTPClient https;
   https.setTimeout(6000);
   https.addHeader("Accept-Encoding", "none", false);
   https.addHeader("Cache-Control", "no-cache");
   https.addHeader("Connection", "keep-alive");
   String url = "http://ipinfo.io/" + ip + "?token=" + IP_Key;
   if (https.begin(client, url))
   {
      int code = 0;
      if ((code = https.GET()) == 200)
      {
         String payload = https.getString();
         DynamicJsonDocument json(payload.length() * 2);
         deserializeJson(json, payload);

         String loc = json["loc"].as<String>();
         int index = loc.indexOf(",") + 1;
         double la = loc.substring(0, index - 1).toDouble();
         double lo = loc.substring(index).toDouble();

         char laStr[10];
         char loStr[10];
         snprintf(laStr, sizeof(laStr), "%.2f", la);
         snprintf(loStr, sizeof(loStr), "%.2f", lo);
         String location = "";
         location += laStr;
         location += ",";
         location += loStr;
         Serial.println(location);
         LittleFS.remove(locFile);
         File f = LittleFS.open(locFile, "w");
         f.print(location);
         f.close();
         Serial.println("获取到经纬度" + String(laStr) + "," + String(loStr));
         // 已经写入经纬度，那么使天气服务进行更新即可
         tWeatherUpdateTask.enable();
      }
      else
      {
         Serial.println("访问IP服务失败");
      }
   }
   tInitLonAndLatTask.disable();
}

void ttimeUpdate() // 1s更新一次
{
   if (tTimeUpdateTask.isFirstIteration())
   {
      Serial.println("展示时间");
   }

   if (!WiFi.isConnected())
   {
      return;
   }
   timeClient.update();
   tft.setTextSize(2);
   String s = timeClient.getFormattedTime();
   int text_width = tft.textWidth(s);
   int x = tft.width() - text_width;
   tft.setCursor(x, 224);
   tft.println(s);
   tft.setTextSize(1);
   yield();
   tTimeUpdateTask.getInternalStatusRequest()->signalComplete();
}
void displayWeather()
{
   if (tWeatherDisplayTask.isFirstIteration())
   {
      Serial.println("展示天气");
   }

   if (wa.text != "")
   {
      tft.setCursor(0, 222);
      tft.println("Temp:" + wa.tmp);
      tft.print("Humi:" + wa.humidity);
   }
   yield();
}

void initWeather()
{
   if (tWeatherUpdateTask.isFirstIteration())
   {
      Serial.println("初始化天气");
   }

   if (!WiFi.isConnected())
   {
      return;
   }

   loc = readLoc();
   if (loc == "")
   {
      return;
   }
   std::unique_ptr<BearSSL::WiFiClientSecure> client(new BearSSL::WiFiClientSecure);
   client->setInsecure();
   HTTPClient https;
   https.addHeader("Accept-Encoding", "none");
   https.addHeader("User-Agent", "Android");
   https.addHeader("Connection", "keep-alive");
   String url = "https://devapi.qweather.com/v7/weather/now?gzip=n&key=" +
                WeatherKey + "&location=" + loc;
   if (https.begin(*client, url))
   {
      int code = https.GET();
      yield();
      if (code == HTTP_CODE_OK || code == HTTP_CODE_MOVED_PERMANENTLY)
      {
         String payload = https.getString();
         DynamicJsonDocument json(payload.length() * 2);
         deserializeJson(json, payload);
         JsonObject obj = json.as<JsonObject>();
         wa.tmp = obj["now"]["temp"].as<String>();
         wa.humidity = obj["now"]["humidity"].as<String>();
         wa.text = obj["now"]["text"].as<String>();
         Serial.println("获取到天气信息");
         tWeatherUpdateTask.getInternalStatusRequest()->signalComplete();
      }
      else
      {
         Serial.println("连接失败" + code);
         tft_Print_Bottom("Initial Weather Failed!");
         tWeatherUpdateTask.getInternalStatusRequest()->signal(-1);
      }
   }
   else
   {
      Serial.println("连接失败");
      tft_Print_Bottom("Initial Weather Failed!");
   }
}

String readLoc() // 经纬度
{
   File f = LittleFS.open(locFile, "r");
   for (size_t i = 0; i < f.size(); i++)
   {
      loc += f.readString();
   }
   f.close();
   return loc;
}
