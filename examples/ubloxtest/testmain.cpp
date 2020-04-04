/**
 * \file    testmain.cpp
 * \brief   Test graphic library
 * \author  Yunhui Fu (yhfudev@gmail.com)
 * \version 1.0
 * \date    2020-01-09
 * \copyright GPL/BSD
 */

#include "osporting.h"
#include "config-hw.h"
#include "config-sw.h"

#include "utils.h"

#define USE_RTC_DS1307 0
#define USE_GPS_PPS    0


////////////////////////////////////////////////////////////////////////////////
#if 0
#undef TD
#define TD(...)
#undef TW
#define TW(...)
#undef TE
#define TE(...)
#undef TI
#define TI(...)
#endif

////////////////////////////////////////////////////////////////////////////////
// for temp test

#undef PIN_GPS_PPS
#undef PIN_GPS_RX
#undef PIN_GPS_TX
#define PIN_GPS_PPS D3
#define PIN_GPS_RX  D4
#define PIN_GPS_TX  D8

#if USE_RTC_DS1307
#include "RTClib.h"
RTC_DS1307 rtc;
#endif

void
setup_rtc()
{
#if USE_RTC_DS1307
  rtc.begin();
  delay(500);
  if (! rtc.isrunning()) {
    TD("RTC is NOT running!");
    // set the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
#endif
}

void show_rtc () {
#if USE_RTC_DS1307
  DateTime now = rtc.now();
  TI("%04d/%02d/%02d %02d:%02d:%02d"
    , now.year()
    , now.month()
    , now.day()
    , now.hour()
    , now.minute()
    , now.second()
    );
  TI("since midnight 1/1/1979 = %lds = %ld d", now.unixtime(), now.unixtime() / 86400L);
    
  // calculate a date which is 7 days and 30 seconds into the future
  DateTime future (now.unixtime() + 7 * 86400L + 30);
  TI("now+7d+30s=%04d/%02d/%02d %02d:%02d:%02d"
    , future.year()
    , future.month()
    , future.day()
    , future.hour()
    , future.minute()
    , future.second()
    );
#endif
}

static unsigned long pre_rtc_tm = 0;
void
loop_rtc()
{
  if (pre_rtc_tm + 3000 < millis()) {
    pre_rtc_tm = millis();
    show_rtc();
  }
}

////////////////////////////////////////////////////////////////////////////////
#if USE_GPS_PPS
volatile unsigned long g_cnt_cycle_bak = 0;
volatile unsigned long g_cnt_cycle[2] = {0, 0};
volatile boolean g_flg_cnt_cycle = false;
volatile int g_idx_cnt_cycle = 0;
void do_count_cycle()
{
  unsigned long cur = ESP.getCycleCount();
  if (g_flg_cnt_cycle) {
    g_cnt_cycle_bak = cur;
    return;
  }
  if (0 == g_idx_cnt_cycle) {
    if (g_cnt_cycle_bak > g_cnt_cycle[1]) {
      g_cnt_cycle[0] = g_cnt_cycle_bak;
      g_cnt_cycle[1] = cur;
      g_cnt_cycle_bak = 0;
      g_flg_cnt_cycle = true;
      //ESP.restart();
      return;
    } else {
      g_cnt_cycle[0] = cur;
    }
  } else {
    g_cnt_cycle[1] = cur;
    g_cnt_cycle_bak = 0;
    g_flg_cnt_cycle = true;
    //ESP.restart();
  }
  g_idx_cnt_cycle = (g_idx_cnt_cycle + 1) % 2;
}

void setup_gps_pps()
{
  pinMode(PIN_GPS_PPS, INPUT_PULLUP);
  attachInterrupt(PIN_GPS_PPS, do_count_cycle, RISING);
}

void loop_gps_pps()
{
  if (g_flg_cnt_cycle) {
    unsigned long cnt = g_cnt_cycle[1] - g_cnt_cycle[0] - 53; // 53 is the software overhead constant
    g_flg_cnt_cycle = false;
    TI("cycle count=%d", cnt);
  }
}
#else
#define setup_gps_pps()
#define loop_gps_pps()
#endif
////////////////////////////////////////////////////////////////////////////////
#if USE_GPS_PPS

#include <SoftwareSerial.h>
SoftwareSerial uart_gps(PIN_GPS_TX, PIN_GPS_RX); // GPS's RX,TX!!

void setup_gps()
{
  uart_gps.begin();
}

void loop_gps()
{
}

#else
#define setup_gps_pps()
#define loop_gps_pps()
#endif

////////////////////////////////////////////////////////////////////////////////
void setup()
{
#if defined(ARDUINO)
  Serial.begin(115200);
  // Wait for USB Serial.
  while (!Serial) {}

  // Read any input
  delay(200);
  while (Serial.read() >= 0) {}
#endif
  assert(0 == 0);

  setup_rand();
  setup_gps_pps();
  setup_rtc();
}

bool flg_quit = false;

void loop(void)
{
  loop_gps_pps();
  loop_rtc();
}

////////////////////////////////////////////////////////////////////////////////

#if ! defined(ARDUINO)
#include <unistd.h>
#define delay(ms) usleep((ms)*1000)
int
main(void)
{
    setup();
    while (! flg_quit) {
        loop();
        //delay(100);
    }
    return 0;
}
#endif

