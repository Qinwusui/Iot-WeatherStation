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
// 创建Websocket服务端
WebSocketsServer wsServer = WebSocketsServer(8080);
// 获取网络时间
// 初始化UDP
WiFiUDP ntpUDP;
// 初始化时间管理器
NTPClient timeClient(ntpUDP, "ntp1.aliyun.com", 60 * 60 * 8, 1000);
// 定义调度器
Scheduler sc;
// 定义一个天气更新任务
Task tWeatherUpdateTask(1000 * 60 * 60, TASK_FOREVER, &initWeather, &sc);
// 定义一个时间更新任务
Task tTimeUpdateTask(200, TASK_FOREVER, &ttimeUpdate, &sc);
// 定义一个时间初始化任务
Task tTimeInitTask(TASK_SECOND, TASK_ONCE, &initTimeClient, &sc);
// 定义一个天气展示任务
Task tWeatherDisplayTask(1000, TASK_FOREVER, &displayWeather, &sc);
// 定义一个WiFi连接任务
Task tWifiConnectTask(1000, TASK_FOREVER, &initWifi, &sc);
// 定义一个文件系统初始化任务
Task tInitLittleFsTask(TASK_IMMEDIATE, TASK_ONCE, &initLitteFs, &sc);
// 定义一个获取经纬的任务
Task tInitLonAndLatTask(TASK_IMMEDIATE, TASK_ONCE, &getPublicIp, &sc);
// 定义一个初始化WebSocketServer的任务
Task tInitWebServerTask(&initWebSocketServer, &sc);
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
uint16_t color_Main = tft.color565(161, 165, 210);

void setup()
{
   Serial.begin(115200);
   dht.begin();

   initTFT();
   initServer();

   Serial.println("执行多任务");
   tInitLittleFsTask.enable();
   tWifiConnectTask.waitFor(tInitLittleFsTask.getInternalStatusRequest(), 1000UL, -1);
   tTimeInitTask.waitFor(tWifiConnectTask.getInternalStatusRequest(), TASK_SECOND, TASK_ONCE);
   tWeatherUpdateTask.waitFor(tWifiConnectTask.getInternalStatusRequest(), TASK_HOUR, -1);
   tWe
   tTimeUpdateTask.waitFor(tTimeInitTask.getInternalStatusRequest(), TASK_MILLISECOND, -1);
   tWeatherDisplayTask.waitFor(tWeatherUpdateTask.getInternalStatusRequest(), TASK_SECOND, -1);
}
void loop()
{
   server.handleClient();
   wsServer.loop();
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
void doSth(uint8_t *s)
{
   String m = (char *)s;
   // displayLogo(m);
}

void webSocketEvent(WStype_t type, uint8_t *payload, size_t length)
{

   switch (type)
   {
   case WStype_DISCONNECTED:
      break;
   case WStype_CONNECTED:
      break;
   case WStype_TEXT:
      doSth(payload);
      break;
   default:
      break;
   }
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
   }
   else
   {
      tft_Print_Bottom("Initiating FileSys faild!");
   }
   Serial.println("LittleFs初始化完成");
   tInitLittleFsTask.getInternalStatusRequest()->signalComplete();
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
   server.on("/put", handleConfigWifi);
   server.on("/loc", handleLocation);
   server.on("/resetLocation", handleResetLocation);
   server.begin();
}
void handleResetLocation()
{
   String reset = server.arg("reset");
   if (reset == "true")
   {
      tft_Print_Bottom("Resetting Location...");
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
      String htm = R"delimiter(
        <html lang="en">

    <head>
    <meta charset="UTF-8">
    <meta http-equiv="X-UA-Compatible" content="IE=edge">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Wrong</title>
    </head>

    <body>
    <span style="text-align: center;">WiFi配置不正确，请点击按钮返回上一页重新填写！</span><br>
    <button id="back">返回</button>
    </body>
    <script>
    var button = document.getElementById("back")
    button.addEventListener("click", function () {
        window.history.back()
    })
    </script>

    </html>
        )delimiter";
      server.send(200, "text/html", htm);
   }
   else
   {
      String wifi = ssid + "||" + pwd;
      File f = LittleFS.open(wifiFile, "w");
      f.print(wifi);
      f.close();
      String html = R"delimiter(
        <html lang="en">
    <head>
    <meta charset="UTF-8">
    <meta http-equiv="X-UA-Compatible" content="IE=edge">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Finished</title>
    </head>
    <body>
    <span style="text-align: center;">已上传WiFi配置，正在配网中，您可以关闭该网页！</span>
    </body>
    </html>
    )delimiter";
      server.send(200, "text/html", html);
      tWifiConnectTask.enable();
   }
}
void handleRoot()
{
   String s = R"delimiter(
        <html lang="en">
    <head>
    <meta charset="UTF-8">
    <meta http-equiv="X-UA-Compatible" content="IE=edge">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Esp8266配网</title>
    </head>
    <body>
    <form action="put">
        <span>输入WIFI SSID：</span><input type="text" name="ssid" id="ssid"><br />
        <span>输入PassWord：</span><input type="text" name="pwd" id="pwd">
        <button type="submit">提交</button>
    </form>
    <span style="text-align: center;"><a href="loc">打开位置配置</a></span>

    </body>
    </html>
    )delimiter";
   server.send(200, "text/html", s);
}
void handleLocation()
{
   String html = R"delimiter(
      <html lang="en">

<head>
   <meta charset="UTF-8">
   <meta http-equiv="X-UA-Compatible" content="IE=edge">
   <meta name="viewport" content="width=device-width, initial-scale=1.0">
   <title>定位重置</title>
</head>

<body>
   <header style="text-align: center;">首先必须保证已经连接好了WiFi，之后点击下方按钮即可开始重新获取定位</header>
   <form action="resetLocation">
      <input type="text" hidden="hidden" name="reset" value="true">
      <button style="text-align: center;">重置</button>
   </form>
</body>

</html>
   )delimiter";
   server.send(200, "text/html", html);
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
      delay(1000);
      tft_Clear_Bottom();
      tWifiConnectTask.getStatusRequest()->signalComplete();
      tWifiConnectTask.disable();
   }
   else // 没有连接成功，重试
   {

      if (tWifiConnectTask.getRunCounter() % 5 == 0) // 每5秒重试一次
      {
         Serial.println("重试中..");
         WiFi.reconnect();
         yield();
      }

      if (tWifiConnectTask.getRunCounter() == 30) // 当到了20秒后，不再重试
      {
         Serial.println("重试了30次了");
         tft_Print_Bottom("Connect" + ssid + "Failed");
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
         location += loStr;
         location += ",";
         location += laStr;
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
void initWebSocketServer()
{
   tft_Print_Bottom("Init WebSocketServer");
   // TODO 创建websocket服务端
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
