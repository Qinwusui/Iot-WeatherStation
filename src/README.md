# 贰零贰贰·腊月拾捌

---

## ```NodeMcu 12-E``` 项目功能简介拆分

### 缘起

- 目前已经大致掌握了ESP8266的软件结构和各个接口的用途，程序也已经写的差不多了，只差UI方面的调整了

- 随着功能越加越多，代码逻辑越来越混乱，亟需对代码进行重构，尽量把各个功能之间的耦合度降低，将可重用代码提取出来，并重新对函数名进行规范

---

### 功能简介

- 连接WIFI
  - 自动连接
    - 自动读取闪存中的WiFi配置信息，并应用到WiFi连接对象中

- 手动连接
  - 建立WebServer
  - 建立AP```无线接入点[Wifi热点]```
    - 手机连接到热点后进入```192.168.4.1```进行配网操作
    - ESP8266支持STA+AP```即WLAN与热点同时开启```的功能，这样可以避免WiFi和热点频繁开闭的问题
- 通过NTP服务器获取网络时间

- 通过网络获取所在城域网IP```伪公网IP```
  
- 通过城域网IP获取大致定位经纬度```定位```

- 通过经纬获取当地天气```实时天气```

- 将所获取到的信息进行展示

---

### 功能拆分

```因为程序代码越来越多，导致各个功能之间依赖性很强，为了解耦合，必须将各个功能进行拆分，保证在每个功能之间耦合度降到最低```

- 使用```TaskScheduler```库进行各个任务间的协调

- 可能会出现任务间的相互依赖关系

  - 如任务A必须等待任务B完成后才能进行

- 任务定时问题

  - 任务可能需要定时进行，比如时间刷新和天气定时更新
