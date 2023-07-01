#include "lvgl.h"
#include <Arduino.h>

static lv_obj_t * meter;
static lv_meter_indicator_t * indic_min;
static lv_meter_indicator_t * indic_hour;
static lv_meter_indicator_t * indic_sec;
static int g_second,g_min,g_hour;
static int g_year,g_month,g_day,g_mday;

void lv_clock_init(int y, int mon, int d, int h, int m, int s)
{
  if(h >= 12) h -= 12;
  g_second = s;
  g_min = m;
  g_hour = h*5;

  g_year = y;
  g_month = mon;
  g_day = d;
}

static void timer_clock_task(lv_timer_t *t)
{
    lv_meter_set_indicator_value(meter,indic_sec,g_second);
    lv_meter_set_indicator_value(meter,indic_min,g_min);
    lv_meter_set_indicator_value(meter,indic_hour,g_hour);
    g_second++;
    if(g_second == 60){
      g_second = 0;
      g_min++;
      if(g_min == 60) {
        g_min = 0;
        g_hour++;
        if(g_hour == 60)
          g_hour=0;
      }
    }
}
void display_date(lv_obj_t *p, int x, int y)
{
  lv_obj_t *labeldate = lv_label_create(lv_scr_act());
  String text;
  text = g_month+1;
  text += "-";
  text += g_day;

  lv_label_set_text(labeldate, text.c_str());
  lv_obj_set_width(labeldate, 60);
  lv_obj_set_style_text_align(labeldate, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_align(labeldate, LV_ALIGN_BOTTOM_LEFT, 0, -10);
}
/**
 * A clock from a meter
 */
void lv_clock_start(int x, int y, int w, int h)
{
    meter = lv_meter_create(lv_scr_act());
    lv_obj_set_size(meter, w, h);
    //lv_obj_center(meter);
    lv_obj_align(meter,LV_ALIGN_CENTER,x,y);

    /*Create a scale for the minutes*/
    /*61 ticks in a 360 degrees range (the last and the first line overlaps)*/
    lv_meter_scale_t * scale_min = lv_meter_add_scale(meter);
    lv_meter_set_scale_ticks(meter, scale_min, 61, 1, 10, lv_palette_main(LV_PALETTE_GREY));
    lv_meter_set_scale_range(meter, scale_min, 0, 60, 360, 270);

    /*Create another scale for the hours. It's only visual and contains only major ticks*/
    lv_meter_scale_t * scale_hour = lv_meter_add_scale(meter);
    lv_meter_set_scale_ticks(meter, scale_hour, 12, 0, 0, lv_palette_main(LV_PALETTE_GREY));               /*12 ticks*/
    lv_meter_set_scale_major_ticks(meter, scale_hour, 1, 2, 20, lv_color_black(), 10);    /*Every tick is major*/
    lv_meter_set_scale_range(meter, scale_hour, 1, 12, 330, 300);       /*[1..12] values in an almost full circle*/

    indic_sec = lv_meter_add_needle_line(meter, scale_min, 2,lv_palette_main(LV_PALETTE_RED),5);
    indic_min = lv_meter_add_needle_line(meter, scale_min, 4,lv_palette_main(LV_PALETTE_GREY),-6);
    indic_hour = lv_meter_add_needle_line(meter, scale_min, 6,lv_palette_main(LV_PALETTE_GREEN),-3);

  display_date(meter,x,y);
  lv_timer_t *timer = lv_timer_create(timer_clock_task, 1000, NULL);
}
