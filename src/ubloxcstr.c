/**
 * \file    ubloxcstr.c
 * \brief   UBlox C strings
 * \author  Yunhui Fu (yhfudev@gmail.com)
 * \version 1.0
 * \date    2018-11-14
 * \copyright GPL/BSD
 */

#include <stdio.h>
#include <assert.h>

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

//#define TD(...)
//#define TI(...)
#define TD printf
#define TI printf
#define TW printf
#define TE printf

#if !defined(DEBUG) || (DEBUG == 0)
#undef TD
#define TD(...)
#undef TI
#define TI(...)

#if 0
#undef TW
#define TW(...)
#undef TE
#define TE(...)
#endif // 0

#endif // DEBUG


/*****************************************************************************/
#define STRCMP_STATIC(buf, static_str) strncmp(buf, static_str, sizeof(static_str)-1)


// is space between items
#define IS_SPACE(a) ( isblank(a) || ('\t' == (a)) || ('\r' == (a)) || ('\n' == (a)) || (0 == (a)) )
//#define IS_SPACE(a) ( isblank(a) || ('\t' == (a)) || ('\r' == (a)) || ('\n' == (a)) )

// is space, and not new line
#define IS_BLANK(a) ( isblank(a) || ('\t' == (a)) )

#if defined(CIUT_ENABLED) && (CIUT_ENABLED == 1)
#include <ciut.h>

TEST_CASE( .name="ublox-IS_BLANK", .description="Test IS_BLANK functions." ) {
    SECTION("test ublox IS_BLANK") {
        REQUIRE(IS_BLANK('\t'));
        REQUIRE(! IS_BLANK('\r'));
        REQUIRE(! IS_BLANK('\n'));
        REQUIRE(IS_BLANK(' '));
        REQUIRE(IS_SPACE('\t'));
        REQUIRE(IS_SPACE('\r'));
        REQUIRE(IS_SPACE('\n'));
        REQUIRE(IS_SPACE(' '));
        REQUIRE(isblank('\t'));
        REQUIRE(! isblank('\r'));
        REQUIRE(! isblank('\n'));
        REQUIRE(isblank(' '));
    }
}
#endif /* CIUT_ENABLED */

/*****************************************************************************/

typedef struct _cstr_val_t {
    const char * cstr;
    uint8_t      val;
} cstr_val_t;

typedef struct _record_cstr_val_t{
    cstr_val_t * cstrval;
    size_t sz_val;
} record_cstr_val_t;

static cstr_val_t list_id_mon[] = {
    {"HW", 0x09},
    {"HW2", 0x0B},
    {"IO", 0x02}, //
    {"MSGPP", 0x06}, //
    {"RXBUF", 0x07}, //
    {"RXR", 0x21}, //
    {"TXBUF", 0x08}, //
    {"VER", 0x04},
};
static cstr_val_t list_id_cfg[] = {
    {"ANT", 0x13},
    {"BATCH", 0x93},
    {"BDS", 0x4A},
    {"CFG", 0x09},
    {"DAT", 0x06},
    {"DGNSS", 0x70},
    {"DYNSEED", 0x85},
    {"EKF", 0x12}, //
    {"ESFGWT", 0x29}, //
    {"ESRC", 0x60}, //
    {"FIXSEED", 0x84},
    {"FXN", 0x0E},
    {"GEOFENCE", 0x69},
    {"GNSS", 0x3E},
    {"HNR", 0x5C},
    {"INF", 0x02},
    {"ITFM", 0x39}, //
    {"LOGFILTER", 0x47},
    {"MSG", 0x01},
    {"NAV5", 0x24},
    {"NAVX5", 0x23},
    {"NMEA", 0x17},
    {"NVS", 0x22}, //
    {"ODO", 0x1E},
    {"PM", 0x32},
    {"PM2", 0x3B}, //
    {"PMS", 0x86},
    {"PRT", 0x00},
    {"PWR", 0x57},
    {"RATE", 0x08},
    {"RINV", 0x34},
    {"RST", 0x04},
    {"RXM", 0x11},
    {"SBAS", 0x16},
    {"SMGR", 0x62},
    {"TMODE", 0x1D}, //
    {"TMODE2", 0x3D}, //
    {"TMODE3", 0x71},
    {"TP", 0x07},
    {"TP5", 0x31},
    {"USB", 0x1B},
};
static cstr_val_t list_id_trk[] = {
    {"D2",   0x06}, // antaris4
    {"D5",   0x0A}, // ublox6 ROM 7.03
    {"MEAS", 0x10}, // ubloxM8 ROM 2.01
    {"SFRB", 0x02}, // ublox6 ROM 7.03
    {"SFRBX", 0x0F}, // ubloxM8 ROM 2.01
};
static cstr_val_t list_id_upd[] = {
    {"DOWNL",  0x01},
    {"EXEC",   0x03},
    {"MEMCPY", 0x04},
    {"SOS",    0x14},
    {"UPLOAD", 0x02},
};
static cstr_val_t list_id_nav[] = {
    {"CLOCK",   0x22},
    {"SVINFO",  0x30},
    {"TIMEBDS", 0x24},
    {"TIMEGAL", 0x25},
    {"TIMEGLO", 0x23},
    {"TIMEGPS", 0x20},
    {"TIMELS",  0x26},
    {"TIMEUTC", 0x21},
};

static cstr_val_t list_class[] = {
    {"ACK", 0x05},
    {"AID", 0x0B},
    {"CFG", 0x06},
    {"ESF", 0x10},
    {"HNR", 0x28},
    {"INF", 0x04},
    {"LOG", 0x21},
    {"MGA", 0x13},
    {"MON", 0x0A},
    {"NAV", 0x01},
    {"RXM", 0x02},
    {"SEC", 0x27},
    {"TRK", 0x03},
    {"UPD", 0x09},
};
static record_cstr_val_t ublox_class_id[] = {
    { NULL, 0, },
    { NULL, 0, },
    { &list_id_cfg, NUM_ARRAY(list_id_cfg), },
    { NULL, 0, },
    { NULL, 0, },
    { NULL, 0, },
    { NULL, 0, },
    { NULL, 0, },
    { &list_id_mon, NUM_ARRAY(list_id_mon), },
    { &list_id_nav, NUM_ARRAY(list_id_nav), },
    { NULL, 0, },
    { NULL, 0, },
    { &list_id_trk, NUM_ARRAY(list_id_trk), },
    { &list_id_upd, NUM_ARRAY(list_id_upd), },
};

/*"data_list[idx] - *data_pin"*/
static int
pf_bsearch_cb_comp_cstrval(void *userdata, size_t idx, void * data_pin)
{
    cstr_val_t * data = (cstr_val_t *)userdata;
    return (strcmp(data[idx].cstr, (char *)data_pin));
}

int
cstr2val_ublox_classid(char * buf, size_t size, uint8_t * p_class, uint8_t *p_id)
{
    char buffer[20];
    char *p = buf;
    size_t idx1;
    size_t idx2;

    assert (NUM_ARRAY(list_class) == NUM_ARRAY(ublox_class_id));
    assert (p_class);
    assert (p_id);

    // split the string
    p = strchr(buf, '-');
    if ((NULL == p) || (buf + size <= p)) {
        // not found
        TW( "warning: not found UBX class-id separator\n");
        return -1;
    }
    memset(buffer, 0, sizeof(buffer));
    strncpy(buffer, buf, size);
    p = (buffer + (p - buf));
    *p = 0;

    if (0 > pf_bsearch_r (list_class, NUM_ARRAY(list_class), pf_bsearch_cb_comp_cstrval, buffer, &idx1)) {
        // not found
        TW( "warning: not found UBX class '%s'\n", buffer);
        return -1;
    }
    *p_class = list_class[idx1].val;
    if ((NULL == ublox_class_id[idx1].cstrval) || (ublox_class_id[idx1].sz_val < 1)) {
        TW( "warning: UBX class '%s' is not setup\n", buffer);
        return -1;
    }
    if (0 > pf_bsearch_r (ublox_class_id[idx1].cstrval, ublox_class_id[idx1].sz_val, pf_bsearch_cb_comp_cstrval, p+1, &idx2)) {
        // not found
        TW( "warning: not found UBX class '%s', id '%s'\n", buffer, p+1);
        return -1;
    }
    *p_id = ublox_class_id[idx1].cstrval[idx2].val;
    return 0;
}

#if defined(CIUT_ENABLED) && (CIUT_ENABLED == 1)
#include <ciut.h>

TEST_CASE( .name="ublox-cstr2val_ublox_classid", .description="Test cstr2val_ublox_classid functions." ) {
    char buffer[10];
    SECTION("test ublox cstr2val_ublox_classid") {
        uint8_t class;
        uint8_t id;

#define CSTR_CLASSID "UPD-DOWNL"
        REQUIRE(0 <= cstr2val_ublox_classid(CSTR_CLASSID, sizeof(CSTR_CLASSID)-1, &class, &id));
        REQUIRE(class == 0x09 && id == 0x01);

#define CSTR_CLASSID "MON-HW2"
        REQUIRE(0 <= cstr2val_ublox_classid(CSTR_CLASSID, sizeof(CSTR_CLASSID)-1, &class, &id));
        REQUIRE(class == 0x0A && id == 0x0B);

#define CSTR_CLASSID "NAV-SVINFO"
        REQUIRE(0 <= cstr2val_ublox_classid(CSTR_CLASSID, sizeof(CSTR_CLASSID)-1, &class, &id));
        REQUIRE(class == 0x01 && id == 0x30);

#define CSTR_CLASSID "TRK-SFRB"
        REQUIRE(0 <= cstr2val_ublox_classid(CSTR_CLASSID, sizeof(CSTR_CLASSID)-1, &class, &id));
        REQUIRE(class == 0x03 && id == 0x02);

#define CSTR_CLASSID "CFG-USB"
        REQUIRE(0 <= cstr2val_ublox_classid(CSTR_CLASSID, sizeof(CSTR_CLASSID)-1, &class, &id));
        REQUIRE(class == 0x06 && id == 0x1B);

#define CSTR_CLASSID "NAV-CLOCK"
        REQUIRE(0 <= cstr2val_ublox_classid(CSTR_CLASSID, sizeof(CSTR_CLASSID)-1, &class, &id));
        REQUIRE(class == 0x01 && id == 0x22);

#define CSTR_CLASSID "NAV-TIMEGLO"
        REQUIRE(0 <= cstr2val_ublox_classid(CSTR_CLASSID, sizeof(CSTR_CLASSID)-1, &class, &id));
        REQUIRE(class == 0x01 && id == 0x23);
    }
    SECTION("test ublox cstr2val_ublox_classid non-exist") {
        uint8_t class;
        uint8_t id;

        id = 0xFF;
#define CSTR_CLASSID "NAV-AOPSTATUS"
        REQUIRE(0 > cstr2val_ublox_classid(CSTR_CLASSID, sizeof(CSTR_CLASSID)-1, &class, &id));
        REQUIRE(class == 0x01 && id == 0xFF);
#define CSTR_CLASSID "ACK-ACK"
        REQUIRE(0 > cstr2val_ublox_classid(CSTR_CLASSID, sizeof(CSTR_CLASSID)-1, &class, &id));
        REQUIRE(class == 0x05 && id == 0xFF);
#define CSTR_CLASSID "SEC-SIGN"
        REQUIRE(0 > cstr2val_ublox_classid(CSTR_CLASSID, sizeof(CSTR_CLASSID)-1, &class, &id));
        REQUIRE(class == 0x27 && id == 0xFF);
    }
}
#endif /* CIUT_ENABLED */

/*****************************************************************************/
#define LIST_CLASSID \
    UBLOX_V2S(UBX_MON_HW) \
    UBLOX_V2S(UBX_MON_HW2) \
    UBLOX_V2S(UBX_MON_IO) \
    UBLOX_V2S(UBX_MON_MSGPP) \
    UBLOX_V2S(UBX_MON_RXBUF) \
    UBLOX_V2S(UBX_MON_RXR) \
    UBLOX_V2S(UBX_MON_TXBUF) \
    UBLOX_V2S(UBX_MON_VER) \
    \
    UBLOX_V2S(UBX_ACK_ACK) \
    UBLOX_V2S(UBX_ACK_NAK) \
    \
    UBLOX_V2S(UBX_RXM_RAW) \
    UBLOX_V2S(UBX_RXM_SFRB) \
    UBLOX_V2S(UBX_RXM_RAWX) \
    UBLOX_V2S(UBX_RXM_SFRBX) \
    \
    UBLOX_V2S(UBX_NAV_TIMEGPS) \
    UBLOX_V2S(UBX_NAV_CLOCK) \
    UBLOX_V2S(UBX_NAV_SVINFO) \
    \
    UBLOX_V2S(UBX_CFG_ANT) \
    UBLOX_V2S(UBX_CFG_BDS) \
    UBLOX_V2S(UBX_CFG_CFG) \
    UBLOX_V2S(UBX_CFG_DAT) \
    UBLOX_V2S(UBX_CFG_EKF) \
    UBLOX_V2S(UBX_CFG_ESFGWT) \
    UBLOX_V2S(UBX_CFG_FXN) \
    UBLOX_V2S(UBX_CFG_GNSS) \
    UBLOX_V2S(UBX_CFG_INF) \
    UBLOX_V2S(UBX_CFG_ITFM) \
    UBLOX_V2S(UBX_CFG_MSG) \
    UBLOX_V2S(UBX_CFG_NAV5) \
    UBLOX_V2S(UBX_CFG_NAVX5) \
    UBLOX_V2S(UBX_CFG_NMEA) \
    UBLOX_V2S(UBX_CFG_NVS) \
    UBLOX_V2S(UBX_CFG_PM) \
    UBLOX_V2S(UBX_CFG_PM2) \
    UBLOX_V2S(UBX_CFG_PRT) \
    UBLOX_V2S(UBX_CFG_RATE) \
    UBLOX_V2S(UBX_CFG_RINV) \
    UBLOX_V2S(UBX_CFG_RXM) \
    UBLOX_V2S(UBX_CFG_SBAS) \
    UBLOX_V2S(UBX_CFG_TMODE) \
    UBLOX_V2S(UBX_CFG_TMODE2) \
    UBLOX_V2S(UBX_CFG_TP) \
    UBLOX_V2S(UBX_CFG_TP5) \
    UBLOX_V2S(UBX_CFG_USB) \
    \
    UBLOX_V2S(UBX_UPD_DOWNL) \
    UBLOX_V2S(UBX_UPD_EXEC) \
    UBLOX_V2S(UBX_UPD_MEMCPY) \
    UBLOX_V2S(UBX_UPD_SOS) \
    UBLOX_V2S(UBX_UPD_UPLOAD) \
    \
    UBLOX_V2S(UBX_TRK_D2) \
    UBLOX_V2S(UBX_TRK_D5) \
    UBLOX_V2S(UBX_TRK_MEAS) \
    UBLOX_V2S(UBX_TRK_SFRB) \
    UBLOX_V2S(UBX_TRK_SFRBX)

const char *
val2cstr_ublox_classid(uint16_t class, uint16_t id)
{
#define UBLOX_V2S(a) case a: return #a;
    switch (UBLOX_CLASS_ID(class,id)) {
            LIST_CLASSID
    }
    return "UNKNOWN_UBX_ID";
#undef UBLOX_V2S
}

#define LIST_PORTID \
    UBLOX_V2S(0, "I2C(DDC)") /* Display Data Channel (DDC) */ \
    UBLOX_V2S(1, "UART1") \
    UBLOX_V2S(2, "UART2") \
    UBLOX_V2S(3, "USB") \
    UBLOX_V2S(4, "SPI")

const char *
val2cstr_ublox_portid(uint16_t port_id)
{
    switch (port_id) {
#define UBLOX_V2S(a,c) case a: return c;
            LIST_PORTID
#undef UBLOX_V2S
    }
    return "UNKNOWN_PORT_ID";
}

#if defined(CIUT_ENABLED) && (CIUT_ENABLED == 1)
#include <ciut.h>

TEST_CASE( .name="ublox-val2cstr", .description="Test ublox val2cstr." ) {

    SECTION("test ublox val2cstr classid") {
        CIUT_LOG ("convert classid=0x%02X, id=0x%02X?", (UBX_MON_VER >> 8) & 0xFF, UBX_MON_VER & 0xFF);
#define UBLOX_V2S(a) REQUIRE(0 == strcmp(val2cstr_ublox_classid((a) >> 8, (a) & 0xFF), #a));
        LIST_CLASSID
#undef UBLOX_V2S
        REQUIRE(0 == strcmp(val2cstr_ublox_classid(0xFF, 0xFF), "UNKNOWN_UBX_ID"));
    }
    SECTION("test ublox val2cstr portid") {
        CIUT_LOG ("convert classid=0x%02X, id=0x%02X?", (UBX_MON_VER >> 8) & 0xFF, UBX_MON_VER & 0xFF);
#define UBLOX_V2S(a,c) REQUIRE(0 == strcmp(val2cstr_ublox_portid(a), c));
        LIST_PORTID
#undef UBLOX_V2S
        REQUIRE(0 == strcmp(val2cstr_ublox_portid(13), "UNKNOWN_PORT_ID"));
    }
}
#endif /* CIUT_ENABLED */

/*****************************************************************************/

ssize_t
cstrlist2array_dec_val(const char *cstr_in, size_t len_cstr, char * buffer_out, size_t sz_bufout)
{
    int i = 0;
    unsigned int val;
    char *p = cstr_in;
    char *p_end = cstr_in + len_cstr;
    while (IS_BLANK(*p) && p < p_end) p ++;
    while ((p_end > p) && IS_SPACE(*(p_end-1))) p_end --;
    if (p >= p_end) {
        return i;
    }

    for (i = 0; ; i ++) {
        while (IS_BLANK(*p) && p < p_end) p ++;
        if (p >= p_end) {
            break;
        }
        if (sscanf(p, "%d", &val) <= 0) {
            break;
        }
        if (sz_bufout < i) {
            TE( "output buffer size too small: '%d' at pos=%d\n", sz_bufout, i);
            return -2;
        }
        // got value
        buffer_out[i] = val & 0xFF;
        while (! IS_BLANK(*p) && p < p_end) p ++;
        if (p >= p_end) {
            i ++;
            break;
        }
    }
    return i;
}

// return < 0 on error, >=0 the size of output buffer
ssize_t
cstrlist2array_hex_val(const char *cstr_in, size_t len_cstr, char * buffer_out, size_t sz_bufout)
{
    int i = 0;
    unsigned int val;
    char *p = cstr_in;
    char *p_end = cstr_in + len_cstr;
    while (IS_BLANK(*p) && p < p_end) p ++;
    while ((p_end > p) && IS_SPACE(*(p_end-1))) p_end --;
    if (p >= p_end) {
        return i;
    }

    for (i = 0; ; i ++) {
        while (IS_BLANK(*p) && p < p_end) p ++;
        if (p >= p_end) {
            break;
        }
        if (sscanf(p, "%x", &val) <= 0) {
            break;
        }
        if (sz_bufout < i) {
            TE( "output buffer size too small: '%d' at pos=%d\n", sz_bufout, i);
            return -2;
        }
        // got value
        buffer_out[i] = val & 0xFF;
        while (! IS_BLANK(*p) && p < p_end) p ++;
        if (p >= p_end) {
            i ++;
            break;
        }
    }
    return i;
}

#if defined(CIUT_ENABLED) && (CIUT_ENABLED == 1)
#include <ciut.h>

TEST_CASE( .name="ublox-cstrlist2array_dec_val", .description="Test cstrlist2array_dec_val functions." ) {
    char buffer[10];
    SECTION("test ublox cstrlist2array_dec_val") {
#define CSTR_DEC "35 204 33 0 0 0 2 16"
#define CSTR_HEX "23 cc 21 00 00 00 02 10"
        char hex_1[] = {0x23, 0xcc, 0x21, 0x00, 0x00, 0x00, 0x02, 0x10,};

        REQUIRE(sizeof(buffer) >= sizeof(hex_1));
        CIUT_LOG ("check cstrlist2array_dec_val %d", 0);
        REQUIRE(sizeof(hex_1) == cstrlist2array_dec_val(CSTR_DEC, sizeof(CSTR_DEC)-1, buffer, sizeof(buffer)));
        REQUIRE(0 == memcmp(buffer, hex_1, sizeof(hex_1)));
        CIUT_LOG ("check cstrlist2array_hex_val %d", 0);
        REQUIRE(sizeof(hex_1) == cstrlist2array_hex_val(CSTR_HEX, sizeof(CSTR_HEX)-1, buffer, sizeof(buffer)));
        REQUIRE(0 == memcmp(buffer, hex_1, sizeof(hex_1)));
#undef CSTR_DEC
#undef CSTR_HEX
    }
    SECTION("test ublox cstrlist2array_dec_val 2") {
#define CSTR_DEC "15 204 33 0 0 0 2 17"
#define CSTR_HEX "0f cc 21 00 00 00 02 11"
        char hex_1[] = {0x0f, 0xcc, 0x21, 0x00, 0x00, 0x00, 0x02, 0x11,};

        REQUIRE(sizeof(buffer) >= sizeof(hex_1));
        CIUT_LOG ("check cstrlist2array_dec_val %d", 0);
        REQUIRE(sizeof(hex_1) == cstrlist2array_dec_val(CSTR_DEC, sizeof(CSTR_DEC)-1, buffer, sizeof(buffer)));
        REQUIRE(0 == memcmp(buffer, hex_1, sizeof(hex_1)));
        CIUT_LOG ("check cstrlist2array_hex_val %d", 0);
        REQUIRE(sizeof(hex_1) == cstrlist2array_hex_val(CSTR_HEX, sizeof(CSTR_HEX)-1, buffer, sizeof(buffer)));
        REQUIRE(0 == memcmp(buffer, hex_1, sizeof(hex_1)));
#undef CSTR_DEC
#undef CSTR_HEX
    }
    SECTION("test ublox cstrlist2array_dec_val") {
#define CSTR_DEC "151 105 33 0 0 0 2 16"
#define CSTR_HEX "97 69 21 00 00 00 02 10"
        char hex_1[] = {0x97, 0x69, 0x21, 0x00, 0x00, 0x00, 0x02, 0x10,};

        REQUIRE(sizeof(buffer) >= sizeof(hex_1));
        CIUT_LOG ("check cstrlist2array_dec_val %d", 0);
        REQUIRE(sizeof(hex_1) == cstrlist2array_dec_val(CSTR_DEC, sizeof(CSTR_DEC)-1, buffer, sizeof(buffer)));
        REQUIRE(0 == memcmp(buffer, hex_1, sizeof(hex_1)));
        CIUT_LOG ("check cstrlist2array_hex_val %d", 0);
        REQUIRE(sizeof(hex_1) == cstrlist2array_hex_val(CSTR_HEX, sizeof(CSTR_HEX)-1, buffer, sizeof(buffer)));
        REQUIRE(0 == memcmp(buffer, hex_1, sizeof(hex_1)));
#undef CSTR_DEC
#undef CSTR_HEX
    }
    SECTION("test ublox cstrlist2array_dec_val 2") {
#define CSTR_DEC "131 105 33 0 0 0 2 17"
#define CSTR_HEX "83 69 21 00 00 00 02 11"
        char hex_1[] = {0x83, 0x69, 0x21, 0x00, 0x00, 0x00, 0x02, 0x11,};

        REQUIRE(sizeof(buffer) >= sizeof(hex_1));
        CIUT_LOG ("check cstrlist2array_dec_val %d", 0);
        REQUIRE(sizeof(hex_1) == cstrlist2array_dec_val(CSTR_DEC, sizeof(CSTR_DEC)-1, buffer, sizeof(buffer)));
        REQUIRE(0 == memcmp(buffer, hex_1, sizeof(hex_1)));
        CIUT_LOG ("check cstrlist2array_hex_val %d", 0);
        REQUIRE(sizeof(hex_1) == cstrlist2array_hex_val(CSTR_HEX, sizeof(CSTR_HEX)-1, buffer, sizeof(buffer)));
        REQUIRE(0 == memcmp(buffer, hex_1, sizeof(hex_1)));
#undef CSTR_DEC
#undef CSTR_HEX
    }
}
#endif /* CIUT_ENABLED */

/*****************************************************************************/

/**
 * \brief process a line and output the binary to buffer for U-Blox config file
 * \param buf_in: the input buffer
 * \param sz_bufin: the size of input buffer
 * \param buf_out: the output buffer
 * \param sz_bufout: the size of output buffer
 *
 * \return >=0 on successs, <0 on error(the command is not recognized)
 *
 */
ssize_t
ublox_confline2bin_hex(char * buf, size_t size, uint8_t * buf_out, size_t sz_bufout)
{
    uint8_t class;
    uint8_t id;
    uint16_t len;
    char *p = buf;
    char *p_end = NULL;
    char *p_next = NULL;
    char buffer[20];
    ssize_t sz_ret;

    if (sz_bufout < 8) {
        TE( "output buffer size too small: '%d'\n", sz_bufout);
        return -2;
    }
    p = buf;
    p_end = buf + size;
    while ((p < p_end) && IS_SPACE(*p)) p ++;

    // find the " - "
    p_end = strstr(p, " - ");
    if ((NULL == p_end) || (p_end >= buf + size)){
        TW("not found ubloxhex separator: '%s'\n", buf);
        return -1;
    }
    p_next = p_end + 2;
    while ((p_end > p) && IS_SPACE(*(p_end-1))) p_end --;
    if (p >= p_end) {
        TW("not found ubloxhex header: '%s'\n", buf);
        return -1;
    }

    assert (sizeof(buffer) > (p_end - p));
    memset(buffer, 0, sizeof(buffer));
    strncpy(buffer, p, (p_end - p));

    if (0 > cstr2val_ublox_classid(buffer, (p_end - p), &class, &id)) {
        // not found
        TW("not found ubloxhex class: '%s'\n", buffer);
        return -1;
    }
    TD("found ubloxhex class: class=%d, id=%d\n", class, id);

    buf_out[0] = 0xB5;
    buf_out[1] = 0x62;
    p = p_next;
    sz_ret = cstrlist2array_hex_val(p, size - (p - buf), buf_out + 2, sz_bufout - 2 - 2);
    if (sz_ret < 0) {
        TE("output buffer size too small: '%d'\n", sz_bufout);
        return -2;
    }
    // check the class,id,length
    len = (((unsigned int)buf_out[5]) << 8) | buf_out[4];
    TD("len=%d, sz_ret=%d; len + 2 + 4=%d, sz_ret + 2=%d\n", len, sz_ret, len + 2 + 4, sz_ret + 2);

    assert (len + 2 + 4 == sz_ret + 2);
    assert (class == buf_out[2]);
    assert (id == buf_out[3]);

    ublox_pkt_checksum(buf_out + 2, sz_ret, buf_out + sz_ret + 2);
    sz_ret += 4;

    return sz_ret;
}

/**
 * \brief process a line and output the binary to buffer for RTKLIB's ublox config file
 * \param buf_in: the input buffer
 * \param sz_bufin: the size of input buffer
 * \param buf_out: the output buffer
 * \param sz_bufout: the size of output buffer
 *
 * \return >=0 on successs, <0 on error(the command is not recognized)
 *
 */
ssize_t
ublox_confline2bin_rtklibarg(char * buf_in, size_t sz_bufin, char * buf_out, size_t sz_bufout)
{
    ssize_t ret = -1;
    uint8_t class;
    uint8_t id;
    char *p = NULL;
    char *p_end = NULL;
    char buf_prefix[20];

#define CSTR_CUR_COMMAND "!UBX"
    if (0 != STRCMP_STATIC (buf_in, CSTR_CUR_COMMAND)) {
        TI("not a rtklib ubx command\n");
        return -1;
    }
    // find the " "
    p_end = buf_in + sz_bufin;
    p = buf_in + sizeof(CSTR_CUR_COMMAND);
    while ((p < p_end) && IS_SPACE(*p)) p ++;
    p_end = strchr(p, ' ');
    if (NULL == p_end) {
        TI("not found space ' '.\n");
        p_end = buf_in + sz_bufin;
    }
    while ((p_end > p) && IS_SPACE(*(p_end-1))) p_end --;

    memset(buf_prefix, 0, sizeof(buf_prefix));
    strncpy(buf_prefix, p, p_end - p);
#undef CSTR_CUR_COMMAND

    if (0 > cstr2val_ublox_classid(buf_prefix, strlen(buf_prefix), &class, &id)) {
        // not found
        TW( "not found rtklibarg class: '%s'\n", buf_prefix);
        return -1;
    }

    p = p_end;
    p_end = buf_in + sz_bufin;
    while ((p_end > p) && IS_SPACE(*(p_end-1))) p_end --;

    switch (UBLOX_CLASS_ID(class,id)) {
        case UBX_MON_VER:
            ret = ublox_pkt_create_get_version (buf_out, sz_bufout);
            break;
        case UBX_MON_HW:
            ret = ublox_pkt_create_get_hw (buf_out, sz_bufout);
            break;
        case UBX_MON_HW2:
            ret = ublox_pkt_create_get_hw2 (buf_out, sz_bufout);
            break;
        case UBX_UPD_DOWNL:
        {
            unsigned int u4_1;
            unsigned int u4_2;
            ssize_t len;
            char buf2[40];

            if (sscanf(p, "%d %d", &u4_1, &u4_2) != 2) {
                // error in scan the data
                TE( "sscanf two numbers failed\n");
                ret = -1;
                break;
            }

            // skip previous blanks
            while (IS_BLANK(*p) && p < p_end) p ++;
            if (p >= p_end) {
                break;
            }
            // skip the first value u4_1
            while (! IS_BLANK(*p) && p < p_end) p ++;
            if (p >= p_end) {
                break;
            }
            // skip the spaces
            while (IS_BLANK(*p) && p < p_end) p ++;
            if (p >= p_end) {
                break;
            }
            // skip the second value u4_2
            while (! IS_BLANK(*p) && p < p_end) p ++;
            if (p >= p_end) {
                break;
            }
            // skip the spaces
            while (IS_BLANK(*p) && p < p_end) p ++;
            if (p >= p_end) {
                break;
            }

            len = cstrlist2array_dec_val(p, sz_bufin - (p - buf_in), buf2, sizeof(buf2));
            if (len < 0) {
                TE( "output buffer size too small: '%d'\n", sizeof(buf2));
                ret = -2;
                break;
            }
            TD("ublox_pkt_create_upd_downl(0x%08X 0x%08X) from '%s'\n", u4_1, u4_2);
            ret = ublox_pkt_create_upd_downl (buf_out, sz_bufout, u4_1, u4_2, buf2, len);
        }
        break;

        case UBX_CFG_BDS:
        {
            unsigned int u4_1;
            unsigned int u4_2;
            unsigned int u4_3;
            unsigned int u4_4;
            unsigned int u4_5;
            unsigned int u4_6;

            sscanf(p, "%d %d %d %d %d %d", &u4_1, &u4_2, &u4_3, &u4_4, &u4_5, &u4_6);
            TD("ublox_pkt_create_cfg_bds(0x%08X 0x%08X 0x%08X 0x%08X 0x%08X 0x%08X) from '%s'\n", u4_1, u4_2, u4_3, u4_4, u4_5, u4_6, p_end);
            ret = ublox_pkt_create_cfg_bds (buf_out, sz_bufout, u4_1, u4_2, u4_3, u4_4, u4_5, u4_6);
        }
        break;

        case UBX_CFG_MSG:
        {
            int i;
            uint8_t buf1[8];
            memset(buf1, 0, sizeof(buf1));

            for (i = 0; i < 8; i ++) {
                // skip the spaces
                while (IS_BLANK(*p) && p < p_end) p ++;
                if (p >= p_end) {
                    break;
                }
                buf1[i] = atoi(p);
                while (! IS_BLANK(*p) && p < p_end) p ++;
            }
            if (i < 2) {
                // error
                break;
            }
            ret = ublox_pkt_create_set_cfgmsg (buf_out, sz_bufout, buf1[0], buf1[1], buf1 + 2, i - 2);
        }
        break;

        case UBX_CFG_PRT:
        {
            unsigned int portID = 0xFF;

            int i;
            char *p1 = p;

            for (i = 0; i < 6; i ++) {
                // skip the spaces
                while (IS_BLANK(*p) && p < p_end) p ++;
                if (p >= p_end) {
                    break;
                }
                if (i == 0) {
                    assert (portID == 0xFF);
                    if (1 != sscanf(p, "%d", &portID)) {
                        assert (portID == 0xFF);
                        break;
                    }
                }
                while (! IS_BLANK(*p) && p < p_end) p ++;
            }
            if (i < 6) {
                // get conf
                ret = ublox_pkt_create_get_cfgprt(buf_out, sz_bufout, portID);
            } else {
                unsigned int txReady;
                unsigned int mode;
                unsigned int baudRate;
                unsigned int inProtoMask;
                unsigned int outProtoMask;
                unsigned int reserved;
                sscanf(p1, "%d %d %d %d %d %d", &portID, &txReady, &mode, &baudRate, &inProtoMask, &outProtoMask);
                ret = ublox_pkt_create_set_cfgprt (buf_out, sz_bufout, portID, txReady, mode, baudRate, inProtoMask, outProtoMask);
            }
        }
        break;

        case UBX_CFG_RATE:
        {
            int i;
            char *p1 = p;

            for (i = 0; i < 3; i ++) {
                // skip the spaces
                while (IS_BLANK(*p) && p < p_end) p ++;
                if (p >= p_end) {
                    break;
                }
                while (! IS_BLANK(*p) && p < p_end) p ++;
            }
            if (i < 3) {
                // get conf
                ret = ublox_pkt_create_get_cfgrate(buf_out, sz_bufout);
            } else {
                unsigned int measRate;
                unsigned int navRate;
                unsigned int timeRef;
                sscanf(p1, "%d %d %d", &measRate, &navRate, &timeRef);
                ret = ublox_pkt_create_set_cfgrate (buf_out, sz_bufout, measRate, navRate, timeRef);
            }
        }
        break;
//ssize_t ublox_pkt_create_set_cfg_gnss (uint8_t *buffer, size_t sz_buf, uint8_t msgVer, uint8_t numTrkChHw, uint8_t numTrkChUse, uint8_t numConfigBlocks, uint8_t *data);

        case UBX_CFG_GNSS:
        {
            unsigned int msgVer;
            unsigned int numTrkChHw;
            unsigned int numTrkChUse;
            unsigned int numConfigBlocks;

            unsigned int gnssId;
            unsigned int resTrkCh;
            unsigned int maxTrkCh;
            unsigned int reserved1;
            unsigned int flags;
            char buffer2[200];

            int i;
            int j;

            if (4 != sscanf(p, "%d %d %d %d", &msgVer, &numTrkChHw, &numTrkChUse, &numConfigBlocks)) {
                // error
                TW("parse CFG-GNSS");
                break;
            }
            for (j = 0; j < 4; j ++) {
                // skip the spaces
                while (IS_BLANK(*p) && p < p_end) p ++;
                if (p >= p_end) {
                    break;
                }
                while (! IS_BLANK(*p) && p < p_end) p ++;
            }
            for (i = 0; i < numConfigBlocks; i ++) {
                assert (8 * i <= sizeof(buffer2));
                if (5 != sscanf(p, "%d %d %d %d %d", &gnssId, &resTrkCh, &maxTrkCh, &reserved1, &flags)) {
                    // error
                    TW("parse CFG-GNSS block");
                    break;
                }
                buffer2[8 * i + 0] = gnssId & 0xFF;
                buffer2[8 * i + 1] = resTrkCh & 0xFF;
                buffer2[8 * i + 2] = maxTrkCh & 0xFF;
                buffer2[8 * i + 3] = 0;
                buffer2[8 * i + 4] = flags & 0xFF;
                buffer2[8 * i + 5] = (flags >> 8) & 0xFF;
                buffer2[8 * i + 6] = (flags >> 16) & 0xFF;
                buffer2[8 * i + 7] = (flags >> 24) & 0xFF;
                for (j = 0; j < 5; j ++) {
                    // skip the spaces
                    while (IS_BLANK(*p) && p < p_end) p ++;
                    if (p >= p_end) {
                        break;
                    }
                    while (! IS_BLANK(*p) && p < p_end) p ++;
                }
            }
            ret = ublox_pkt_create_set_cfg_gnss (buf_out, sz_bufout, msgVer, numTrkChHw, numTrkChUse, numConfigBlocks, buffer2);
        }
        break;

        default:
            ret = -1;
            break;
    }

    return ret;
}

#if defined(CIUT_ENABLED) && (CIUT_ENABLED == 1)
#include <ciut.h>
//ssize_t ublox_confline2bin_rtklibarg(char * buf_in, size_t sz_bufin, char * buf_out, size_t sz_bufout);
TEST_CASE( .name="ublox-ublox_confline2bin_rtklibarg", .description="Test ublox_confline2bin_rtklibarg functions." ) {
    char buffer1[50];
    char buffer2[50];
    int i;
    static char * list_test_cases[][3] = {
        { // dec, hex, out
            "!UBX UPD-DOWNL 4060 0   35 204 33 0 0 0 2 16",
            "UPD-DOWNL - 09 01 10 00 dc 0f 00 00 00 00 00 00 23 cc 21 00 00 00 02 10",
            "b5 62 09 01 10 00 dc 0f 00 00 00 00 00 00 23 cc 21 00 00 00 02 10 27 0e",
        },
        {
            "!UBX UPD-DOWNL 4360 0   15 204 33 0 0 0 2 17",
            "UPD-DOWNL - 09 01 10 00 08 11 00 00 00 00 00 00 0f cc 21 00 00 00 02 11",
            "b5 62 09 01 10 00 08 11 00 00 00 00 00 00 0f cc 21 00 00 00 02 11 42 4d",
        },
        {
            "!UBX UPD-DOWNL 6412 0  131 105 33 0 0 0 2 17",
            "UPD-DOWNL - 09 01 10 00 0c 19 00 00 00 00 00 00 83 69 21 00 00 00 02 11",
            "b5 62 09 01 10 00 0c 19 00 00 00 00 00 00 83 69 21 00 00 00 02 11 5f f0",
        },
        {
            "!UBX UPD-DOWNL 5832 0  151 105 33 0 0 0 2 16",
            "UPD-DOWNL - 09 01 10 00 c8 16 00 00 00 00 00 00 97 69 21 00 00 00 02 10",
            "b5 62 09 01 10 00 c8 16 00 00 00 00 00 00 97 69 21 00 00 00 02 10 2b 22",
        },
        {
            "!UBX CFG-MSG 3 15 0 1 0 1 0 0",
            " CFG-MSG - 06 01 08 00 03 0F 00 01 00 01 00 00",
            "b5 62 06 01 08 00 03 0F 00 01 00 01 00 00 23 2C",
        },
        {
            "!UBX MON-VER",
            " MON-VER - 0A 04 00 00",
            "b5 62 0A 04 00 00 0e 34",
        },
        {
            "!UBX MON-HW ",
            "MON-HW - 0A 09 00 00 ",
            "b5 62 0A 09 00 00 13 43",
        },
        {
            "!UBX \t  CFG-RATE \r\n",
            " \t  CFG-RATE   - \t 06 08 00 00\r\n",
            "b5 62 06 08 00 00 0e 30",
        },
        {
            "!UBX CFG-PRT ",
            "CFG-PRT - 06 00 00 00 ",
            "b5 62 06 00 00 00 06 18",
        },
        {
            "!UBX CFG-PRT  1 ",
            "CFG-PRT - 06 00 01 00 01 ",
            "b5 62 06 00 01 00 01 08 22",
        },
        {
            "!UBX CFG-BDS 0  0    31  4294967295  0  0",
            "CFG-BDS - 06 4A 18 00 00 00 00 00 00 00 00 00 1F 00 00 00 FF FF FF FF 00 00 00 00 00 00 00 00",
            "B5 62 06 4A 18 00 00 00 00 00 00 00 00 00 1F 00 00 00 FF FF FF FF 00 00 00 00 00 00 00 00 83 AC",
        },
        {
            "!UBX CFG-PRT    \t  2  ",
            "CFG-PRT   -  \t 06 00 01 00 02  ",
            "b5 62 06 00 01 00 02 09 23",
        },
        {
            "!UBX \t CFG-GNSS   0 32 32 1   6 16 16 0  65537   \r\n",
            "\t CFG-GNSS  -   06 3E 0C 00 00 20 20 01 06 10 10 00 01 00 01 00  \r\n",
            "b5 62 06 3E 0C 00 00 20 20 01 06 10 10 00 01 00 01 00 B9 59",
        },
        {
            "!UBX \t   CFG-GNSS 0 0 0 0   \r\n",
            "\t   CFG-GNSS -  06 3E 04 00 00 00 00 00   \r\n",
            "b5 62 06 3E 04 00 00 00 00 00 48 FA",
        },
    };
    SECTION("test ublox_confline2bin") {
        for (i = 0; i < NUM_ARRAY(list_test_cases); i ++) {
            REQUIRE(sizeof(buffer1) >= (strlen(list_test_cases[i][2]) + 2)/3);
            REQUIRE((strlen(list_test_cases[i][2]) + 2)/3 == ublox_confline2bin_rtklibarg(list_test_cases[i][0], strlen(list_test_cases[i][0]), buffer1, sizeof(buffer1)));
            REQUIRE((strlen(list_test_cases[i][2]) + 2)/3 == cstrlist2array_hex_val(list_test_cases[i][2], strlen(list_test_cases[i][2]), buffer2, sizeof(buffer2)));
            CIUT_LOG("---------------- [%d]\n", i);
            CIUT_LOG("buffer1:", 0);
            hex_dump_to_fd(STDERR_FILENO, (opaque_t *)(buffer1), (strlen(list_test_cases[i][2]) + 2)/3);
            CIUT_LOG("buffer2:", 0);
            hex_dump_to_fd(STDERR_FILENO, (opaque_t *)(buffer2), (strlen(list_test_cases[i][2]) + 2)/3);
            CIUT_LOG("----------------", 0);
            REQUIRE(0 == memcmp(buffer1, buffer2, (strlen(list_test_cases[i][2]) + 2)/3));
            REQUIRE((strlen(list_test_cases[i][2]) + 2)/3 == ublox_confline2bin_hex(list_test_cases[i][1], strlen(list_test_cases[i][1]), buffer1, sizeof(buffer1)));
            REQUIRE(0 == memcmp(buffer1, buffer2, (strlen(list_test_cases[i][2]) + 2)/3));
        }
    }
}
#endif /* CIUT_ENABLED */
