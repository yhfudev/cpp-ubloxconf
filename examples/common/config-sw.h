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
#define USE_MENU   1

#define USE_SDL 1
#if defined(ARDUINO)
#undef USE_SDL
#define USE_SDL 0
#endif


#define SHOW_DEMO_MENU_ITEMS 1 // demo MenuItem
#define SHOW_DEMO_MENU_GFX   0
#define SHOW_DEMO_MENU_ABOUT 1
#if (USE_ADAGFX==1) || (USE_LCDGFX==1) || (USE_ST7920==1) || (USE_EMBGFX==1)
#undef  SHOW_DEMO_MENU_GFX
#define SHOW_DEMO_MENU_GFX   1
#endif


#endif // _CONFIG_SW_UBLOXCONF_H
