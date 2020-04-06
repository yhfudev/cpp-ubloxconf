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

#include "ubloxconn.h"
#include "ubloxcstr.h"

#if DEBUG
#include "hexdump.h"
#endif
#ifndef hex_dump_to_fd
#define hex_dump_to_fd(a,b,c)
#endif
#ifndef hex_dump_to_fp
#define hex_dump_to_fp(a,b,c)
#endif

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
#if ARDUINO_ARCH_ESP8266
#undef PIN_GPS_PPS
#undef PIN_GPS_RX
#undef PIN_GPS_TX
#define PIN_GPS_PPS D3
#define PIN_GPS_RX  D0 //D4
#define PIN_GPS_TX  D8
#endif

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

#if defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
ICACHE_RAM_ATTR
#endif
void
do_count_cycle()
{
#if defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
  unsigned long cur = ESP.getCycleCount();
#else
  unsigned long cur = millis();
#endif
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
  attachInterrupt(digitalPinToInterrupt(PIN_GPS_PPS), do_count_cycle, RISING);
}

void loop_gps_pps()
{
  if (g_flg_cnt_cycle) {
    unsigned long cnt;
    cnt = g_cnt_cycle[1] - g_cnt_cycle[0] - 53; // 53 is the software overhead constant
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

#if ARDUINO_ARCH_ESP32
#define uart_gps Serial2
#else
#include <SoftwareSerial.h>
SoftwareSerial uart_gps(PIN_GPS_TX, PIN_GPS_RX); // GPS's RX,TX!!
#endif

#define BAUDRATE_DEFAULT 9600L
#define BAUDRATE_WORKING 115200L
static const unsigned long g_gps_uart_baudrates[] = {
    //921600L,
    //460800L,
    //230400L,
    BAUDRATE_WORKING, //115200L,
    //57600L,
    //38400L,
    //19200L,
#if BAUDRATE_WORKING != BAUDRATE_DEFAULT
    BAUDRATE_DEFAULT, // 9600L,
#endif
    //4800L,
};

static void disable_nmea(Stream * p_uart)
{
#define ITEM(a) {UBLOX_2CLASS(a), UBLOX_2ID(a)}
  static uint8_t class_id_nmea[][2] = {
    ITEM(UBX_NMEA_GxGGA),
    ITEM(UBX_NMEA_GxGLL),
    ITEM(UBX_NMEA_GxGSA),
    ITEM(UBX_NMEA_GxGSV),
    ITEM(UBX_NMEA_GxRMC),
    ITEM(UBX_NMEA_GxVTG),
    ITEM(UBX_NMEA_GxGRS),
    ITEM(UBX_NMEA_GxGST),
    ITEM(UBX_NMEA_GxZDA),
    ITEM(UBX_NMEA_GxGBS),
    ITEM(UBX_NMEA_GxDTM),
    ITEM(UBX_NMEA_GxGNS),
    ITEM(UBX_NMEA_GxTHS),
    ITEM(UBX_NMEA_GxVLW),
    ITEM(UBX_PUBX_POS),
    ITEM(UBX_PUBX_01),
    ITEM(UBX_PUBX_SV),
    ITEM(UBX_PUBX_TIME),
    ITEM(UBX_PUBX_POS2),
    ITEM(UBX_PUBX_POS3),
  };
#undef ITEM
  uint8_t buffer[8+3];
  ssize_t sz_buf;
  uint8_t rate = 0;
  for (int i = 0; i < sizeof(class_id_nmea)/sizeof(*class_id_nmea); i ++) {
    // set current rate
    sz_buf = ublox_pkt_create_set_cfgmsg(buffer, sizeof(buffer), class_id_nmea[i][0], class_id_nmea[i][1], &rate, 1);
    if (sz_buf <= 0) {
      TE("ublox pkt error");
      continue;
    }
    p_uart->write (buffer, sz_buf);
  }
}

void
restore_work_baudrate()
{
  uint8_t buffer[8+25];
  ssize_t sz_buf;
  int i;

  TD("reset to saved config");
  sz_buf = ublox_pkt_create_set_cfgcfg(buffer, sizeof(buffer), 0x1F1F, 0x00, 0x1F1F, 0x17);
  for (i = 0; (sz_buf > 0) && (i < NUM_ARRAY(g_gps_uart_baudrates)); i ++) {
    TD("reset UART @ %d", g_gps_uart_baudrates[i]);
    delay(100);
    uart_gps.flush();
    uart_gps.begin(g_gps_uart_baudrates[i]);
    uart_gps.write(buffer, sz_buf);
  }

  delay(100);
  if (g_gps_uart_baudrates[NUM_ARRAY(g_gps_uart_baudrates)-1] != BAUDRATE_DEFAULT) {
    TD("set to default baudrate");
    uart_gps.flush();
    uart_gps.begin(BAUDRATE_DEFAULT);
    delay(100);
  }

#if BAUDRATE_WORKING != BAUDRATE_DEFAULT
  TD("switch GPS UART to working baudrate");
  sz_buf = ublox_pkt_create_set_cfgprt(buffer, sizeof(buffer), 0x01, 0x00, 0x000008D0, BAUDRATE_WORKING, 0x0027, 0x0023);
  if (sz_buf <= 0) {
    TE("ublox pkt error");
  } else {
    uart_gps.write(buffer, sz_buf);
    delay(100);
    uart_gps.flush();
    uart_gps.begin(BAUDRATE_WORKING);
    delay(100);
  }
#endif
  delay(100);
  //uart_gps.flush();
  TI("U-Blox GPS setup done.");
}

void
setup_ublox_config_lines(Stream * p_uart, const char *lines[], size_t num_lines)
{
  ssize_t sz_buf;
  int i;
  char buffer[50];
  for (i = 0; i < num_lines; i ++) {
    TD("%s", lines[i]);
    sz_buf = ublox_confline2bin_rtklibarg(lines[i], strlen(lines[i]), buffer, sizeof(buffer));
    if (sz_buf <= 0) {
      TW("ignore the line '%s' due to parse error.", lines[i]);
      continue;
    }
    p_uart->write(buffer, sz_buf);
  }
}

void
setup_ublox6_703()
{
  static const char * setup_strings_ublox_common[] = {
    // change the measRate=100(0x64)ms=10Hz, navRate=1cylc
    "!UBX CFG-RATE 100 1 1",

    "!UBX CFG-GNSS 0 32 32 1 0 10 32 0 1",
    "!UBX CFG-GNSS 0 32 32 1 6 8 16 0 1"
  };
  static const char * setup_strings_ublox6_703[] = {
    // ublox6 ROM7.03
    "!UBX UPD-DOWNL 5832 0  151 105 33 0 0 0 2 16",
    "!UBX UPD-DOWNL 6412 0  131 105 33 0 0 0 2 17",

    // TRK-TRKD5+TRK-SFRB+NAV-CLOCK+NAV-TIMEGPS Parser
    // GPS-only, UART1 and USB:
    "!UBX CFG-MSG 3  2 0 1 0 1 0 0",
    "!UBX CFG-MSG 3 10 0 1 0 1 0 0",
    "!UBX CFG-MSG 3 15 0 1 0 1 0 0",
    "!UBX CFG-MSG 3 16 0 1 0 1 0 0",
    "!UBX CFG-MSG 1 32 0 1 0 1 0 0",
    "!UBX CFG-MSG 1 34 0 1 0 1 0 0",
    // enable NAV-CLOCK
    "!UBX CFG-MSG 1 34 0 1 0 1 0 0",
  };

  TD("Disable NMEA"); disable_nmea(&uart_gps);
  TD("setup U-Blox common");
  setup_ublox_config_lines(&uart_gps, setup_strings_ublox_common, NUM_ARRAY(setup_strings_ublox_common));
  TD("setup U-Blox 6 v7.03");
  setup_ublox_config_lines(&uart_gps, setup_strings_ublox6_703, NUM_ARRAY(setup_strings_ublox6_703));
}

void setup_gps()
{
  uart_gps.begin(BAUDRATE_DEFAULT);
  restore_work_baudrate();

  setup_ublox6_703();

  ssize_t sz_buf;
  uint8_t buffer[50];
  TD("get version");
  sz_buf = ublox_pkt_create_get_version(buffer, sizeof(buffer));
  assert (sz_buf > 0);
  uart_gps.write(buffer, sz_buf);

  TD("get HW");
  sz_buf = ublox_pkt_create_get_hw(buffer, sizeof(buffer));
  assert (sz_buf > 0);
  uart_gps.write(buffer, sz_buf);

  TD("get HW2");
  sz_buf = ublox_pkt_create_get_hw2(buffer, sizeof(buffer));
  assert (sz_buf > 0);
  uart_gps.write(buffer, sz_buf);

  TD("get prt");
  sz_buf = ublox_pkt_create_get_cfgprt(buffer, sizeof(buffer), 1);
  assert (sz_buf > 0);
  uart_gps.write(buffer, sz_buf);

  TD("get rate");
  sz_buf = ublox_pkt_create_get_cfgrate(buffer, sizeof(buffer));
  assert (sz_buf > 0);
  uart_gps.write(buffer, sz_buf);
}

uint8_t g_buffer_gps[1200];
size_t sz_buf_gps = 0;
void loop_gps()
{
  int ret;
  size_t sz_processed = 0;
  size_t sz_needed_in = 0;
  while (uart_gps.available() && sz_buf_gps < sizeof(g_buffer_gps)) {
    g_buffer_gps[sz_buf_gps] = uart_gps.read();
    sz_buf_gps ++;
    //Serial.print('.');
    assert (sz_buf_gps <= sizeof(g_buffer_gps));
    if (sz_buf_gps % 11 == 0) {
      // to break the loop for 'Interrupt wdt timeout on CPU1'
      break;
    }
  }
  if (sz_buf_gps < 1) {
    return;
  }
  ret = ublox_process_buffer_data(g_buffer_gps, sz_buf_gps, &sz_processed, &sz_needed_in);
  if (sz_processed > 0) {
    TI("current buffer:");
    hex_dump_to_fp(stderr, g_buffer_gps, sz_buf_gps);
    TD("buffer advanced %d", sz_processed);
    if (sz_buf_gps > sz_processed) {
      memmove(g_buffer_gps, g_buffer_gps + sz_processed, sz_buf_gps - sz_processed);
      sz_buf_gps -= sz_processed;
    } else {
      sz_buf_gps = 0;
    }
  }
  if (sz_needed_in + sz_buf_gps > sizeof(g_buffer_gps)) {
    TI("current buffer:");
    hex_dump_to_fp(stderr, g_buffer_gps, sz_buf_gps);
    TW("ignore the current since no enough buffer, szbuf=%d+needed=%d > sz_g_buf=%d", sz_buf_gps, sz_needed_in, sizeof(g_buffer_gps));
    memmove(g_buffer_gps, g_buffer_gps + 1, sz_buf_gps - 1);
    sz_buf_gps --;
  }
  if (ret != 0) {
    return;
  }
}

#else
#define setup_gps()
#define loop_gps()
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
  setup_gps();
  setup_rtc();
}

bool flg_quit = false;

void loop(void)
{
  loop_gps_pps();
  loop_gps();
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

