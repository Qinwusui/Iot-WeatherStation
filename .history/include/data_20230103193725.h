#include <Arduino.h>
typedef struct WeatherData
{
   String tmp;      // 温度
   String text;     // 天气描述
   String humidity; // 相对湿度
} weather;