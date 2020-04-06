/**
 * \file    config-sw.h
 * \brief   Config of software
 * \author  Yunhui Fu (yhfudev@gmail.com)
 * \version 1.0
 * \date    2020-01-09
 * \copyright GPL/BSD
 */
#ifndef _CONFIG_SW_UBLOXCONF_H
#define _CONFIG_SW_UBLOXCONF_H 1

#include "config-hw.h"

////////////////////////////////////////////////////////////////////////////////
#define CSTR_PROGNAME "U-Blox GPS config"
//#define CSTR_PROGNAME "Thermostat"
#define CSTR_PROGVER "0.1.1"

////////////////////////////////////////////////////////////////////////////////

#define USE_SDL 1
#if defined(ARDUINO)
#undef USE_SDL
#define USE_SDL 0
#endif


#if defined(ARDUINO)
#define USE_RTC_DS1307 1
#define USE_GPS_PPS    1
#else
#define USE_RTC_DS1307 0
#define USE_GPS_PPS    0
#endif

#endif // _CONFIG_SW_UBLOXCONF_H
