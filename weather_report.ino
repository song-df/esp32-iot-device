#include <Arduino.h>
#include "rm67162.h"
#include "pins_config.h"
#include "WiFi.h"
#include "lvgl.h"
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "zones.h"
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

LV_FONT_DECLARE(SourceHanSerifSC_SB);
LV_FONT_DECLARE(JetBrainsMono_thin);

struct tm timeinfo;
void lv_clock_init(int y, int mon, int d, int h, int m, int s);
void lv_clock_start(int x, int y, int w, int h);

char City[] = "杭州.   ";
unsigned char temp = 0;
char Weather[] = "晴          ";
unsigned char weather_code = 0;
int g_year = 0;
unsigned char g_month = 0;
unsigned char g_day = 0;
unsigned char g_wday = 0;
const long gmtOffset_sec = 8 * 3600;
const int daylightOffset_sec = 0;
static lv_obj_t *wifi_info_label1;

void wifi_setup() {
  String text;
  uint32_t last_tick = millis();
  uint32_t i = 0;
  bool is_smartconfig_connect = false;

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(1000);
  int n = WiFi.scanNetworks();
  if (n == 0) {
    Serial.println("no networks found");
    return;
  } else {
    text = n;
    text += " networks found\n";
    for (int i = 0; i < n; ++i) {
      text += (i + 1);
      text += ": ";
      text += WiFi.SSID(i);
      text += " (";
      text += WiFi.RSSI(i);
      text += ")";
      text += (WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? " \n" : "*\n";
      delay(10);
    }
    lv_label_set_text(wifi_info_label1, text.c_str());
    lv_timer_handler();
    delay(1);
    Serial.println(text);
  }

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  delay(3000);
  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    if (millis() - last_tick > WIFI_CONNECT_WAIT_MAX) {
      is_smartconfig_connect = true;
      WiFi.mode(WIFI_AP_STA);
      WiFi.beginSmartConfig();
      while (1) {
        delay(100);
        if (WiFi.smartConfigDone()) {
          Serial.println("\r\nSmartConfig Success\r\n");
          Serial.printf("SSID:%s\r\n", WiFi.SSID().c_str());
          Serial.printf("PSW:%s\r\n", WiFi.psk().c_str());
          last_tick = millis();
          break;
        }
      }
    }
  }
  if (!is_smartconfig_connect) {
    Serial.print("\n CONNECTED \nTakes ");
    Serial.print(millis() - last_tick);
    Serial.println(" millseconds");
  }
  delay(2000);
}
const char *ntpServer = "pool.ntp.org";
String tagetHttp = "https://api.seniverse.com/v3/weather/now.json?key=SbPafAepQpIKHm3c9&location=ip&language=zh-Hans&unit=c";
DynamicJsonDocument doc(1024);
DeserializationError error;


static lv_disp_draw_buf_t draw_buf;
static lv_color_t *buf;
static lv_obj_t *meter;
static lv_obj_t *dis;


void printLocalTime() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("No time available (yet)");
    return;
  }
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
  lv_clock_init(timeinfo.tm_year,timeinfo.tm_mon,timeinfo.tm_mday,timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
  g_day = timeinfo.tm_yday;
  g_month = timeinfo.tm_mon;
  g_year = timeinfo.tm_year;
  g_wday = timeinfo.tm_wday;
}

void time_init() {
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
}

void getCityandWeather() {
  HTTPClient http;

  http.setTimeout(5000);
  http.begin(tagetHttp);
  int httpCode = http.GET();
  if (httpCode > 200) {
    Serial.println("Weather Service unavailable.");
  }
  if (httpCode == HTTP_CODE_OK) {
    String resBuf = http.getString();
    //Serial.println(resBuf);
    error = deserializeJson(doc, resBuf);

    strcpy(City, doc["results"][0]["location"]["name"]);
    temp = doc["results"][0]["now"]["temperature"];
    strcpy(Weather, doc["results"][0]["now"]["text"]);
    Serial.printf("temp=%d, weather=%s,City is %s\n", temp, Weather, City);
    http.end();
  }
}

void led_task(void *param) {
  int cnt = 0;
  //pinMode(PIN_LED, OUTPUT);
  while (1) {
    //digitalWrite(PIN_LED, 1);
    delay(20);

    //digitalWrite(PIN_LED, 0);
    delay(980);

    if (cnt == 5) {
      cnt = 0;
      if (WiFi.status() == WL_CONNECTED)
        getCityandWeather();
    }
    cnt++;
  }
}


void my_disp_flush(lv_disp_drv_t *disp,
                   const lv_area_t *area,
                   lv_color_t *color_p) {
  uint32_t w = (area->x2 - area->x1 + 1);
  uint32_t h = (area->y2 - area->y1 + 1);
  lcd_PushColors(area->x1, area->y1, w, h, (uint16_t *)&color_p->full);
  lv_disp_flush_ready(disp);
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  rm67162_init();
  lcd_setRotation(1);
  //lcd_PushColors(0, 0, 536, 240, (uint16_t *)gImage_haizei_1);

  //xTaskCreatePinnedToCore(led_task, "led_task", 1024, NULL, 1, NULL, 0);

  lv_init();
  buf = (lv_color_t *)ps_malloc(sizeof(lv_color_t) * LVGL_LCD_BUF_SIZE);
  lv_disp_draw_buf_init(&draw_buf, buf, NULL, LVGL_LCD_BUF_SIZE);


  /*Initialize the display*/
  static lv_disp_drv_t disp_drv;
  lv_disp_drv_init(&disp_drv);
  /*Change the following line to your display resolution*/
  disp_drv.hor_res = EXAMPLE_LCD_H_RES;
  disp_drv.ver_res = EXAMPLE_LCD_V_RES;
  disp_drv.flush_cb = my_disp_flush;
  disp_drv.draw_buf = &draw_buf;
  lv_disp_drv_register(&disp_drv);

  wifi_info_label1 = lv_label_create(lv_scr_act());
  lv_obj_set_style_text_font(wifi_info_label1, &SourceHanSerifSC_SB, 0);
  lv_obj_set_width(wifi_info_label1, 500); /*Set smaller width to make the lines wrap*/
  lv_obj_set_style_text_align(wifi_info_label1, LV_TEXT_ALIGN_LEFT, 10);
  lv_obj_align(wifi_info_label1, LV_ALIGN_TOP_LEFT, 10, 10);


  //LV_IMG_DECLARE(haizeix_logo_1);
  //lv_img_set_src(lv_img_create(lv_scr_act()),&haizeix_logo_1);

  wifi_setup();
  if (WiFi.status() == WL_CONNECTED)
    getCityandWeather();




  LV_IMG_DECLARE(bg_6);

  lv_obj_t *img = lv_img_create(lv_scr_act());
  lv_img_set_src(img, &bg_6);
  //lv_obj_set_width(img, 100);
  //lv_obj_set_height(img, 100);
  //lv_img_set_angle(img,0);
  lv_obj_align(img, LV_ALIGN_TOP_LEFT, 0, 0);
  lv_obj_set_size(img, 536, 240);

  lv_obj_t *label1 = lv_label_create(lv_scr_act());
  lv_label_set_long_mode(label1, LV_LABEL_LONG_WRAP); /*Break the long lines*/
  lv_label_set_recolor(label1, true);                 /*Enable re-coloring by commands in the text*/
  String text;
  text = "#ff0000 ";
  text += City;
  text += "\n ";
  text += temp;
  text += "℃  ";
  text += Weather;
  lv_label_set_text(label1, text.c_str());
  lv_obj_set_style_text_font(label1, &SourceHanSerifSC_SB, 0);
  lv_obj_set_width(label1, 270); /*Set smaller width to make the lines wrap*/
  lv_obj_set_style_text_align(label1, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_align(label1, LV_ALIGN_TOP_MID, 100, 40);

  lv_obj_t *label2 = lv_label_create(lv_scr_act());
  lv_label_set_long_mode(label2, LV_LABEL_LONG_SCROLL_CIRCULAR); /*Circular scroll*/
  lv_obj_set_width(label2, 250);
  lv_obj_set_style_text_font(label2, &SourceHanSerifSC_SB, 0);
  lv_label_set_text(label2, "请注意高温天气，外出记得带伞!");
  lv_obj_align(label2, LV_ALIGN_BOTTOM_RIGHT, -20, -12);

  //flush display
  lv_timer_handler();
  delay(1);

  time_init();
  printLocalTime();
  WiFi.disconnect();
  lv_clock_start(-150, 0, 200, 200);

  //delay(5000);
  //ui_init();
}

void loop() {
  static unsigned long previousMillis = 0; // 上一次函数调用的时间
  const unsigned long interval = 300000;   // 5分钟的时间间隔（毫秒）

  unsigned long currentMillis = millis();  // 获取当前的毫秒数

  // 检查是否达到了5分钟的时间间隔
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;  // 更新上一次函数调用的时间
    if (WiFi.status() == WL_CONNECTED)
      getCityandWeather();  // 调用需要执行的函数
  }

  // put your main code here, to run repeatedly:
  //if(WiFi.status() == WL_CONNECTED){
  //Serial.println("Try to get weather...");
  //getCityandWeather();

  //}
  lv_timer_handler();
  delay(2);
}





