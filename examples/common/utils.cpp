/**
 * \file    utils.cpp
 * \brief   some misc functions
 * \author  Yunhui Fu (yhfudev@gmail.com)
 * \version 1.0
 * \date    2020-01-09
 * \copyright GPL/BSD
 */

#include "osporting.h"
#include "utils.h"

#if defined(ARDUINO)
#include "TimeLib.h"
#else
#define setTime(hr, mt, sc, dy, mo, yr)
#endif

const char mon_01[] PROGMEM = "Jan";
const char mon_02[] PROGMEM = "Feb";
const char mon_03[] PROGMEM = "Mar";
const char mon_04[] PROGMEM = "Apr";
const char mon_05[] PROGMEM = "May";
const char mon_06[] PROGMEM = "Jun";
const char mon_07[] PROGMEM = "Jul";
const char mon_08[] PROGMEM = "Aug";
const char mon_09[] PROGMEM = "Sep";
const char mon_10[] PROGMEM = "Oct";
const char mon_11[] PROGMEM = "Nov";
const char mon_12[] PROGMEM = "Dec";
const char * const month_list[] PROGMEM = {
  mon_01, mon_02, mon_03, mon_04, mon_05, mon_06,
  mon_07, mon_08, mon_09, mon_10, mon_11, mon_12,
};

void get_time_from_built(char *buf_ret, size_t sz_buf)
{
  // set the current time to the built time
  char buf_date[] = __DATE__;
  char buf_time[] = __TIME__;
  int yr = 2000;
  int mo = 1;
  int dy = 1;
  int hr = 0;
  int mt = 0;
  int sc = 0;
  char *p;

  TD(".");
  p = buf_date;
  for(int i = 0; i < NUM_ARRAY(month_list); i ++) {
    TD("i=%d", i);
    if (0 == strncmp_P(p, (char*)pgm_read_dword(&(month_list[i])), 3)) {
      mo = i + 1;
      break;
    }
  }
  TD(",");
  p = strchr(p, ' ');
  while (p && *p && isblank(*p)) p ++;
  if (p) {
    dy = atoi(p);
  }
  TD(";");
  p = strchr(p, ' ');
  while (p && *p && isblank(*p)) p ++;
  if (p) {
    yr = atoi(p);
  }

  TD("!");
  hr = atoi(buf_time);

  mt = atoi(buf_time + 3);
  sc = atoi(buf_time + 6);
  TD("?");
  setTime(hr, mt, sc, dy, mo, yr);
  //TD("buf_date='%s'; year=%d", buf_date, yr);
  //TD("output: %04d-%02d-%02d %02d:%02d:%02d", yr, mo, dy, hr, mt, sc);
#if 1
  //snprintf_P(buf_ret, sz_buf, PSTR("%04d-%02d-%02d %02d:%02d:%02d"), yr, mo, dy, hr, mt, sc);
  snprintf_P(buf_ret, sz_buf, PSTR("%04d%02d%02d %02d%02d%02d"), yr, mo, dy, hr, mt, sc);
#endif
}

////////////////////////////////////////////////////////////////////////////////

void setup_rand()
{
#if defined(ARDUINO)
  randomSeed(analogRead(0));
#else
  srand(time(NULL));
#endif
#ifndef __AVR__
  {
    struct timeval tv = {0, 0};
    struct timezone tz = {0, 0};
    gettimeofday(&tv, &tz);
    TW("gettimeofday() return tv={%ld.%06d}; tz={%d,%d}.", tv.tv_sec, tv.tv_usec, tz.tz_minuteswest, tz.tz_dsttime);
    delay(100);
    gettimeofday(&tv, &tz);
    TW("after delay, gettimeofday() return tv={%ld.%06d}; tz={%d,%d}.", tv.tv_sec, tv.tv_usec, tz.tz_minuteswest, tz.tz_dsttime);
  }
#endif
}
