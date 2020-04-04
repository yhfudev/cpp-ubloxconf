/**
 * \file    ubloxtest.ino
 * \brief   test RTC
 * \author  Yunhui Fu (yhfudev@gmail.com)
 * \version 1.0
 * \date    2020-01-09
 * \copyright GPL/BSD
 */
#if defined(ARDUINO)
#if ARDUINO >= 100
#include <Arduino.h>
#else
#include <WProgram.h>
#endif
#include <SPI.h>
#include <Wire.h>
//#include <SoftwareSerial.h>
#else
#include <unistd.h>
#define delay(a) usleep(a)
#endif

