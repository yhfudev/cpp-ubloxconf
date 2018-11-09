/**
 * \file    ubloxconn.c
 * \brief   UBlox module connection
 * \author  Yunhui Fu (yhfudev@gmail.com)
 * \version 1.0
 * \date    2018-11-06
 * \copyright GPL/BSD
 */

#include <stdio.h>
#include <assert.h>

#include "ubloxconn.h"

#if DEBUG
#include "hexdump.h"
#endif
#ifndef hex_dump_to_fd
#define hex_dump_to_fd(a,b,c)
#endif
#ifndef hex_dump_to_fp
#define hex_dump_to_fp(a,b,c)
#endif


/*****************************************************************************/

#define UBX_NAV_TIMEGPS 0x0120
#define UBX_NAV_CLOCK   0x0122

#define UBX_RXM_RAW   0x0210
#define UBX_RXM_SFRB  0x0211
#define UBX_RXM_SFRBX 0x0213
#define UBX_RXM_RAWX  0x0215

#define UBX_TRK_D5    0x030A
#define UBX_TRK_MEAS  0x0310
#define UBX_TRK_SFRBX 0x030F

#define UBX_ACK_NAK  0x0500
#define UBX_ACK_ACK  0x0501

#define UBX_CFG_PRT  0x0600
#define UBX_CFG_MSG  0x0601
#define UBX_CFG_RATE 0x0608

#define UBX_DBG_SET  0x0901

#define UBX_MON_VER  0x0A04
#define UBX_MON_HW   0x0A09
#define UBX_MON_HW2  0x0A0B
#define UBX_MON_RXR  0x0A21


#define UBLOX_PKG_LENGTH(p) ((unsigned int)((p)[4]) | (((unsigned int)((p)[5])) << 8))

#define UBLOX_CLASS_ID(class,id) (((class) << 8) | (id))

const char *
ublox_val2cstr_classid(uint16_t class, uint16_t id)
{
#define UBLOX_V2S(a) case a: return #a
    switch (UBLOX_CLASS_ID(class,id)) {
        UBLOX_V2S(UBX_MON_VER);
        UBLOX_V2S(UBX_MON_HW);
        UBLOX_V2S(UBX_MON_HW2);
        UBLOX_V2S(UBX_MON_RXR);

        UBLOX_V2S(UBX_ACK_ACK);
        UBLOX_V2S(UBX_ACK_NAK);

        UBLOX_V2S(UBX_RXM_RAW);
        UBLOX_V2S(UBX_RXM_SFRB);
        UBLOX_V2S(UBX_RXM_RAWX);
        UBLOX_V2S(UBX_RXM_SFRBX);

        UBLOX_V2S(UBX_NAV_TIMEGPS);
        UBLOX_V2S(UBX_NAV_CLOCK);

        UBLOX_V2S(UBX_CFG_PRT);
        UBLOX_V2S(UBX_CFG_MSG);
        UBLOX_V2S(UBX_CFG_RATE);

        UBLOX_V2S(UBX_DBG_SET);

        UBLOX_V2S(UBX_TRK_D5);
        UBLOX_V2S(UBX_TRK_MEAS);
        UBLOX_V2S(UBX_TRK_SFRBX);
    }
    return "UNKNOWN_UBX_ID";
#undef UBLOX_V2S
}

const char *
ublox_val2cstr_portid(uint16_t port_id)
{
    switch (port_id) {
    case 0: return "I2C(DDC)"; // Display Data Channel (DDC)
    case 1: return "UART1";
    case 2: return "UART2";
    case 3: return "USB";
    case 4: return "SPI";
    }
    return "UNKNOWN_PORT_ID";
}


#if defined(CIUT_ENABLED) && (CIUT_ENABLED == 1)
#include <ciut.h>

TEST_CASE( .name="ublox-val2cstr", .description="Test ublox val2cstr." ) {

    SECTION("test ublox val2cstr") {
        CIUT_LOG ("convert classid=0x%02X, id=0x%02X?", (UBX_MON_VER >> 8) & 0xFF, UBX_MON_VER & 0xFF);
        REQUIRE(0 == strcmp(ublox_val2cstr_classid(UBX_MON_VER >> 8, UBX_MON_VER & 0xFF), "UBX_MON_VER"));
        REQUIRE(0 == strcmp(ublox_val2cstr_classid(UBX_MON_HW >> 8, UBX_MON_HW & 0xFF), "UBX_MON_HW"));
    }
}
#endif /* CIUT_ENABLED */


/*****************************************************************************/
/**
 * \brief calculate the checksum of UBlox protocol packet
 * \param buffer:   the buffer contains the packet
 * \param length:   the byte size of the packet
 * \param out_buf:  2-byte length buffer to store the checksum
 * \return the checksum value
 */
void
ublox_pkt_checksum(void *buffer, int length, char * out_buf)
{
    char *chk_a = out_buf;
    char *chk_b = out_buf + 1;

    int i;
    assert (NULL != buffer);
    *chk_a = 0;
    *chk_b = 0;
    for (i = 0; i < length; i++) {
        *chk_a += ((unsigned char*) buffer)[i];
        *chk_b += *chk_a;
    }
}

/**
 * \brief verify if the packet is correct
 * \param buffer:   the buffer contains the packet
 * \param sz_buf:   the byte size of the packet
 * \return <0 on fail, 0 on OK
 */
int
ublox_pkt_verify (uint8_t *buffer, size_t sz_buf)
{
    uint16_t count;
    uint8_t chksum[2];

    if (sz_buf < 8) {
        fprintf(stderr, "Verify error: mini packet size.\n");
        return -1;
    }
    count = UBLOX_PKG_LENGTH(buffer);
    ublox_pkt_checksum(buffer + 2, 4 + count, chksum);
    if ((chksum[0] == *(buffer + 6 + count)) && (chksum[1] == *(buffer + 6 + count + 1))) {
        return 0;
    }

    fprintf(stderr, "Verify error: checksum. sz_buf=%d,count=%d, expected=0x%02X%02X, got=0x%02X%02X\n", sz_buf, count, *(buffer + 6 + count), *(buffer + 6 + count + 1), chksum[0], chksum[1]);
    return -1;
}

#if defined(CIUT_ENABLED) && (CIUT_ENABLED == 1)
#include <ciut.h>

TEST_CASE( .name="ublox-checksum", .description="Test ublox checksum functions." ) {
    char buffer[8 + 300];
    memset(buffer, 0, sizeof(buffer));
    buffer[0] = 0xB5;
    buffer[1] = 0x62;
    buffer[2] = 0x0A;
    buffer[3] = 0x04;
    buffer[4] = 0x00;
    buffer[5] = 0x00;
    ublox_pkt_checksum(buffer + 2, 4, buffer + 6);

    SECTION("test ublox checksum") {
        CIUT_LOG ("check ublox checksum %d", 0);
        REQUIRE(*(buffer + 6) == 0x0E);
        REQUIRE(*(buffer + 7) == 0x34);
        REQUIRE(ublox_pkt_verify(buffer, 8) == 0);
    }
    SECTION("test ublox length") {
        REQUIRE(UBLOX_PKG_LENGTH(buffer) == 0);

        buffer[4] = 0x01;
        buffer[5] = 0x00;
        CIUT_LOG ("check ublox pkt length=%d", UBLOX_PKG_LENGTH(buffer));
        REQUIRE(UBLOX_PKG_LENGTH(buffer) == 1);
        ublox_pkt_checksum(buffer + 2, 4 + UBLOX_PKG_LENGTH(buffer), buffer + 6 + UBLOX_PKG_LENGTH(buffer));
        REQUIRE(ublox_pkt_verify(buffer, 8 + UBLOX_PKG_LENGTH(buffer)) == 0);

        buffer[4] = 0x02;
        buffer[5] = 0x00;
        REQUIRE(UBLOX_PKG_LENGTH(buffer) == 2);
        ublox_pkt_checksum(buffer + 2, 4 + UBLOX_PKG_LENGTH(buffer), buffer + 6 + UBLOX_PKG_LENGTH(buffer));
        REQUIRE(ublox_pkt_verify(buffer, 8 + UBLOX_PKG_LENGTH(buffer)) == 0);

        buffer[4] = 0x10;
        buffer[5] = 0x00;
        CIUT_LOG ("check ublox pkt length=%d", UBLOX_PKG_LENGTH(buffer));
        REQUIRE(UBLOX_PKG_LENGTH(buffer) == 16);
        ublox_pkt_checksum(buffer + 2, 4 + UBLOX_PKG_LENGTH(buffer), buffer + 6 + UBLOX_PKG_LENGTH(buffer));
        REQUIRE(ublox_pkt_verify(buffer, 8 + UBLOX_PKG_LENGTH(buffer)) == 0);

        buffer[4] = 0x12;
        buffer[5] = 0x00;
        REQUIRE(UBLOX_PKG_LENGTH(buffer) == 18);
        ublox_pkt_checksum(buffer + 2, 4 + UBLOX_PKG_LENGTH(buffer), buffer + 6 + UBLOX_PKG_LENGTH(buffer));
        REQUIRE(ublox_pkt_verify(buffer, 8 + UBLOX_PKG_LENGTH(buffer)) == 0);

        buffer[4] = 0x14;
        buffer[5] = 0x00;
        REQUIRE(UBLOX_PKG_LENGTH(buffer) == 20);
        ublox_pkt_checksum(buffer + 2, 4 + UBLOX_PKG_LENGTH(buffer), buffer + 6 + UBLOX_PKG_LENGTH(buffer));
        REQUIRE(ublox_pkt_verify(buffer, 8 + UBLOX_PKG_LENGTH(buffer)) == 0);


        buffer[4] = 0x00;
        buffer[5] = 0x01;
        CIUT_LOG ("check ublox pkt length=%d", UBLOX_PKG_LENGTH(buffer));
        REQUIRE(UBLOX_PKG_LENGTH(buffer) == 256);
        ublox_pkt_checksum(buffer + 2, 4 + UBLOX_PKG_LENGTH(buffer), buffer + 6 + UBLOX_PKG_LENGTH(buffer));
        REQUIRE(ublox_pkt_verify(buffer, 8 + UBLOX_PKG_LENGTH(buffer)) == 0);

        buffer[4] = 0x02;
        buffer[5] = 0x01;
        REQUIRE(UBLOX_PKG_LENGTH(buffer) == 258);
        ublox_pkt_checksum(buffer + 2, 4 + UBLOX_PKG_LENGTH(buffer), buffer + 6 + UBLOX_PKG_LENGTH(buffer));
        REQUIRE(ublox_pkt_verify(buffer, 8 + UBLOX_PKG_LENGTH(buffer) + 2) == 0);

        buffer[4] = 0x04;
        buffer[5] = 0x01;
        REQUIRE(UBLOX_PKG_LENGTH(buffer) == 260);
        ublox_pkt_checksum(buffer + 2, 4 + UBLOX_PKG_LENGTH(buffer), buffer + 6 + UBLOX_PKG_LENGTH(buffer));
        REQUIRE(ublox_pkt_verify(buffer, 8 + UBLOX_PKG_LENGTH(buffer) + 10000) == 0);
    }
}
#endif /* CIUT_ENABLED */

/*****************************************************************************/

/**
 * \brief fill the buffer with the 'get version' packet
 * \param buffer:   the buffer to be filled
 * \param sz_buf:   the byte size of the buffer
 * \return <0 on fail, >0 the size of packet
 */
ssize_t
ublox_pkt_create_get_version (uint8_t *buffer, size_t sz_buf)
{
    ssize_t ret = 0;

    if (NULL == buffer) {
        return -1;
    }
    if (sz_buf < 8) {
        return -1;
    }
    ret = 8;
    assert (NULL != buffer);

    // header
    buffer[0] = 0xB5;
    buffer[1] = 0x62;
    // class
    buffer[2] = 0x0A;
    // ID
    buffer[3] = 0x04;
    // length, little endian
    buffer[4] = 0x00;
    buffer[5] = 0x00;
    // no payload

#if 0
    // checksum A
    buffer[6] = 0x0E;
    // checksum B
    buffer[7] = 0x34;
#else
    ublox_pkt_checksum(buffer + 2, 4, buffer + 6);
#endif

    return ret;
}

/**
 * \brief fill the buffer with the 'MON-HW' packet
 * \param buffer:   the buffer to be filled
 * \param sz_buf:   the byte size of the buffer
 * \return <0 on fail, >0 the size of packet
 */
ssize_t
ublox_pkt_create_get_hw (uint8_t *buffer, size_t sz_buf)
{
    ssize_t ret = 0;

    if (NULL == buffer) {
        return -1;
    }
    if (sz_buf < 8) {
        return -1;
    }
    ret = 8;
    assert (NULL != buffer);

    // header
    buffer[0] = 0xB5;
    buffer[1] = 0x62;
    // class
    buffer[2] = 0x0A;
    // ID
    buffer[3] = 0x09;
    // length, little endian
    buffer[4] = 0x00;
    buffer[5] = 0x00;
    // no payload

    ublox_pkt_checksum(buffer + 2, 4, buffer + 6);

    return ret;
}

/**
 * \brief fill the buffer with the 'MON-HW2' packet
 * \param buffer:   the buffer to be filled
 * \param sz_buf:   the byte size of the buffer
 * \return <0 on fail, >0 the size of packet
 */
ssize_t
ublox_pkt_create_get_hw2 (uint8_t *buffer, size_t sz_buf)
{
    ssize_t ret = 0;

    if (NULL == buffer) {
        return -1;
    }
    if (sz_buf < 8) {
        return -1;
    }
    ret = 8;
    assert (NULL != buffer);

    // header
    buffer[0] = 0xB5;
    buffer[1] = 0x62;
    // class
    buffer[2] = 0x0A;
    // ID
    buffer[3] = 0x0B;
    // length, little endian
    buffer[4] = 0x00;
    buffer[5] = 0x00;
    // no payload

    ublox_pkt_checksum(buffer + 2, 4, buffer + 6);

    return ret;
}

#if defined(CIUT_ENABLED) && (CIUT_ENABLED == 1)
#include <ciut.h>

TEST_CASE( .name="ublox-version", .description="Test ublox ublox_pkt_create_get_version functions." ) {
    char buffer[10];
    SECTION("test ublox ublox_pkt_create_get_version") {
        CIUT_LOG ("check ublox_pkt_create_get_version %d", 0);
        REQUIRE(8 == ublox_pkt_create_get_version(buffer, sizeof(buffer)));
        REQUIRE(0 == UBLOX_PKG_LENGTH(buffer));
        REQUIRE(ublox_pkt_verify(buffer, 8) == 0);
    }
}
#endif /* CIUT_ENABLED */


/**
 * \brief fill the buffer with the 'DBG-SET' packet
 * \param buffer:   the buffer to be filled
 * \param sz_buf:   the byte size of the buffer
 * \return <0 on fail, >0 the size of packet
 */
//!UBX DBG-SET 000016C8 00000000 00216997 0000 02 10
ssize_t
ublox_pkt_create_dbg_set (uint8_t *buffer, size_t sz_buf, uint32_t u4_1, uint32_t u4_2, uint32_t u4_3, uint16_t u2_1, uint8_t class, uint8_t id)
{
    ssize_t ret = 0;

    if (NULL == buffer) {
        return -1;
    }
    if (sz_buf < 8) {
        return -1;
    }
    ret = 8;
    assert (NULL != buffer);

    // header
    buffer[0] = 0xB5;
    buffer[1] = 0x62;
    // class
    buffer[2] = 0x09;
    // ID
    buffer[3] = 0x01;
    // length, little endian
    buffer[4] = 0x10; // 16 bytes
    buffer[5] = 0x00;

    // payload
    ret = 6;
    // little endian
    buffer[ret ++] = u4_1 & 0xFF;
    buffer[ret ++] = (u4_1 >> 8) & 0xFF;
    buffer[ret ++] = (u4_1 >> 16) & 0xFF;
    buffer[ret ++] = (u4_1 >> 24) & 0xFF;

    buffer[ret ++] = u4_2 & 0xFF;
    buffer[ret ++] = (u4_2 >> 8) & 0xFF;
    buffer[ret ++] = (u4_2 >> 16) & 0xFF;
    buffer[ret ++] = (u4_2 >> 24) & 0xFF;

    buffer[ret ++] = u4_3 & 0xFF;
    buffer[ret ++] = (u4_3 >> 8) & 0xFF;
    buffer[ret ++] = (u4_3 >> 16) & 0xFF;
    buffer[ret ++] = (u4_3 >> 24) & 0xFF;

    buffer[ret ++] = u2_1 & 0xFF;
    buffer[ret ++] = (u2_1 >> 8) & 0xFF;
    buffer[ret ++] = class;
    buffer[ret ++] = id;

    ublox_pkt_checksum(buffer + 2, ret - 2, buffer + ret);
    ret += 2;
    assert(ret == 24);

    return ret;
}


/**
 * \brief fill the buffer with the 'UBX-CFG-MSG' packet
 * \param buffer:   the buffer to be filled
 * \param sz_buf:   the byte size of the buffer
 * \return <0 on fail, >0 the size of packet
 */
ssize_t
ublox_pkt_create_set_cfgmsg (uint8_t *buffer, size_t sz_buf, uint8_t class, uint8_t id, uint8_t *rates, int num_rates)
{
    ssize_t ret = 0;

    if (NULL == buffer) {
        return -1;
    }
    if (sz_buf < 8) {
        return -1;
    }
    if ((num_rates != 1) && (num_rates != 6)) {
        return -1;
    }
    ret = 8;
    assert (NULL != buffer);

    fprintf(stderr, "ublox_pkt_create_set_cfgmsg(class=%d, id=%d, reats[]=%d,%d,%d,%d,%d,%d\n", class, id, rates[0], rates[1], rates[2], rates[3], rates[4], rates[5]);

    // header
    buffer[0] = 0xB5;
    buffer[1] = 0x62;
    // class
    buffer[2] = 0x06;
    // ID
    buffer[3] = 0x01;
    // length, little endian
    if (NULL == rates) {
        num_rates = 0;
    }
    assert(num_rates <= 6);
    buffer[4] = 2 + num_rates;
    buffer[5] = 0x00;

    // payload
    ret = 6;
    buffer[ret ++] = class;
    buffer[ret ++] = id;
    if (num_rates > 0) {
        memmove(buffer + ret, rates, num_rates);
        ret += num_rates;
    }

    ublox_pkt_checksum(buffer + 2, ret - 2, buffer + ret);
    ret += 2;
    assert(ret == 8 + 2 + num_rates);

    return ret;
}

/**
 * \brief fill the buffer with the 'UBX-CFG-PRT' packet
 * \param buffer:   the buffer to be filled
 * \param sz_buf:   the byte size of the buffer
 * \param port_id:  the port id, 0xFF - void, 0 - I2C, 1 - UART1, 2 - UART2, 3 - USB, 4 - SPI
 * \return <0 on fail, >0 the size of packet
 */
ssize_t
ublox_pkt_create_get_cfgprt (uint8_t *buffer, size_t sz_buf, uint8_t port_id)
{
    ssize_t ret = 0;

    if (NULL == buffer) {
        return -1;
    }
    if (sz_buf < 8) {
        return -1;
    }
    assert (NULL != buffer);

    // header
    buffer[0] = 0xB5;
    buffer[1] = 0x62;
    // class
    buffer[2] = 0x06;
    // ID
    buffer[3] = 0x00;

    ret = 6;
    // length, little endian
    if (0xFF == port_id) {
        buffer[4] = 0x00;
        buffer[5] = 0x00;
        // payload
    } else {
        if (sz_buf < 9) {
            // no enough buffer
            return -1;
        }
        buffer[4] = 0x01; // 1 bytes
        buffer[5] = 0x00;
        // payload
        buffer[ret ++] = port_id;
    }

    ublox_pkt_checksum(buffer + 2, ret - 2, buffer + ret);
    ret += 2;

    return ret;
}

ssize_t
ublox_pkt_create_set_cfgprt (uint8_t *buffer, size_t sz_buf, uint8_t port_id, uint16_t txReady, uint32_t mode, uint32_t baudRate, uint16_t inPortoMask, uint16_t outPortoMask)
{
    ssize_t ret = 0;

    if (NULL == buffer) {
        return -1;
    }
    if (sz_buf < 8 + 20) {
        return -1;
    }
    assert (NULL != buffer);

    // header
    buffer[0] = 0xB5;
    buffer[1] = 0x62;
    // class
    buffer[2] = 0x06;
    // ID
    buffer[3] = 0x00;

    // length, little endian
    buffer[4] = 20;
    buffer[5] = 0x00;

    // payload
    ret = 6;
    buffer[ret ++] = port_id;
    buffer[ret ++] = 0x00; // reserved0

    buffer[ret ++] = txReady & 0xFF;
    buffer[ret ++] = (txReady >> 8) & 0xFF;

    buffer[ret ++] = mode & 0xFF;
    buffer[ret ++] = (mode >> 8) & 0xFF;
    buffer[ret ++] = (mode >> 16) & 0xFF;
    buffer[ret ++] = (mode >> 24) & 0xFF;

    buffer[ret ++] = baudRate & 0xFF;
    buffer[ret ++] = (baudRate >> 8) & 0xFF;
    buffer[ret ++] = (baudRate >> 16) & 0xFF;
    buffer[ret ++] = (baudRate >> 24) & 0xFF;

    buffer[ret ++] = inPortoMask & 0xFF;
    buffer[ret ++] = (inPortoMask >> 8) & 0xFF;

    buffer[ret ++] = outPortoMask & 0xFF;
    buffer[ret ++] = (outPortoMask >> 8) & 0xFF;

    buffer[ret ++] = 0x00; // reserved4
    buffer[ret ++] = 0x00;
    buffer[ret ++] = 0x00; // reserved5
    buffer[ret ++] = 0x00;

    ublox_pkt_checksum(buffer + 2, ret - 2, buffer + ret);
    ret += 2;

    return ret;
}


/**
 * \brief fill the buffer with the 'UBX-CFG-RATE' packet
 * \param buffer:   the buffer to be filled
 * \param sz_buf:   the byte size of the buffer
 * \return <0 on fail, >0 the size of packet
 */
ssize_t
ublox_pkt_create_get_cfgrate (uint8_t *buffer, size_t sz_buf)
{
    ssize_t ret = 0;

    if (NULL == buffer) {
        return -1;
    }
    if (sz_buf < 8) {
        return -1;
    }
    assert (NULL != buffer);

    // header
    buffer[0] = 0xB5;
    buffer[1] = 0x62;
    // class
    buffer[2] = 0x06;
    // ID
    buffer[3] = 0x08;

    // length, little endian
    ret = 6;
    buffer[4] = 0x00;
    buffer[5] = 0x00;

    ublox_pkt_checksum(buffer + 2, ret - 2, buffer + ret);
    ret += 2;

    return ret;
}

ssize_t
ublox_pkt_create_set_cfgrate (uint8_t *buffer, size_t sz_buf, uint16_t measRate, uint16_t navRate, uint16_t timeRef)
{
    ssize_t ret = 0;

    if (NULL == buffer) {
        return -1;
    }
    if (sz_buf < 8 + 20) {
        return -1;
    }
    assert (NULL != buffer);

    // header
    buffer[0] = 0xB5;
    buffer[1] = 0x62;
    // class
    buffer[2] = 0x06;
    // ID
    buffer[3] = 0x08;

    // length, little endian
    buffer[4] = 6;
    buffer[5] = 0x00;

    // payload
    ret = 6;

    buffer[ret ++] = measRate & 0xFF;
    buffer[ret ++] = (measRate >> 8) & 0xFF;

    buffer[ret ++] = navRate & 0xFF;
    buffer[ret ++] = (navRate >> 8) & 0xFF;

    buffer[ret ++] = timeRef & 0xFF;
    buffer[ret ++] = (timeRef >> 8) & 0xFF;

    ublox_pkt_checksum(buffer + 2, ret - 2, buffer + ret);
    ret += 2;

    return ret;
}

#if defined(CIUT_ENABLED) && (CIUT_ENABLED == 1)
#include <ciut.h>

TEST_CASE( .name="ublox-ext", .description="Test ublox ublox_pkt_create_get_version functions." ) {
    uint8_t buffer[30];
    SECTION("test ublox ublox_pkt_create_dbg_set") {
        CIUT_LOG ("check ublox_pkt_create_dbg_set %d", 0);

        REQUIRE(sizeof(buffer) > 24);
        //# RAW  b5 62 09 01 10 00 c8 16 00 00 00 00 00 00 97 69 21 00 00 00 02 10 2b 22
        REQUIRE(24 == ublox_pkt_create_dbg_set(buffer, sizeof(buffer), 0x000016C8, 0x00000000, 0x00216997, 0x0000, UBX_RXM_RAW >> 8, UBX_RXM_RAW & 0xFF));
        CIUT_LOG("%d RAW buffer:", 0);
        hex_dump_to_fd(STDERR_FILENO, buffer, 24);
        REQUIRE(16 == UBLOX_PKG_LENGTH(buffer));
        REQUIRE(ublox_pkt_verify(buffer, 24) == 0);
        REQUIRE(*(buffer + 24-2) == 0x2B);
        REQUIRE(*(buffer + 24-1) == 0x22);

        //# SFRB b5 62 09 01 10 00 0c 19 00 00 00 00 00 00 83 69 21 00 00 00 02 11 5f f0
        REQUIRE(24 == ublox_pkt_create_dbg_set(buffer, sizeof(buffer), 0x0000190C, 0x00000000, 0x00216983, 0x0000, UBX_RXM_SFRB >> 8, UBX_RXM_SFRB & 0xFF));
        CIUT_LOG("%d SFRB buffer:", 0);
        hex_dump_to_fd(STDERR_FILENO, buffer, 24);
        REQUIRE(16 == UBLOX_PKG_LENGTH(buffer));
        REQUIRE(ublox_pkt_verify(buffer, 24) == 0);
        REQUIRE(*(buffer + 24-2) == 0x5F);
        REQUIRE(*(buffer + 24-1) == 0xF0);
    }

    SECTION("test sscanf") {
        unsigned int u4_1;
        unsigned int u4_2;
        unsigned int u4_3;
        unsigned int u2_1;
        unsigned int class;
        unsigned int id;

#define CSTR_CUR_COMMAND "!UBX DBG-SET"
        char buf[256] = "!UBX DBG-SET 000016C8 00000000 00216997 0000 02 10";
        sscanf(buf + sizeof(CSTR_CUR_COMMAND), "%X %X %X %X %X %X", &u4_1, &u4_2, &u4_3, &u2_1, &class, &id);
        fprintf(stderr, "got (0x%08X 0x%08X 0x%08X 0x%04X 0x%02X 0x%02X) from '%s'\n", u4_1, u4_2, u4_3, u2_1, class, id, buf + sizeof(CSTR_CUR_COMMAND));
        REQUIRE(u4_1 == 0x000016C8);
        REQUIRE(u4_2 == 0x00000000);
        REQUIRE(u4_3 == 0x00216997);
        REQUIRE(u2_1 == 0x0000);
        REQUIRE(class == 0x02);
        REQUIRE(id == 0x10);

        strcpy(buf, "!UBX DBG-SET 0000190C 00000000 00216983 0000 02 11");
        sscanf(buf + sizeof(CSTR_CUR_COMMAND), "%x %X %X %X %X %X", &u4_1, &u4_2, &u4_3, &u2_1, &class, &id);
        fprintf(stderr, "got (0x%08X 0x%08X 0x%08X 0x%04X 0x%02X 0x%02X) from '%s'\n", u4_1, u4_2, u4_3, u2_1, class, id, buf + sizeof(CSTR_CUR_COMMAND));
        REQUIRE(u4_1 == 0x0000190C);
        REQUIRE(u4_2 == 0x00000000);
        REQUIRE(u4_3 == 0x00216983);
        REQUIRE(u2_1 == 0x0000);
        REQUIRE(class == 0x02);
        REQUIRE(id == 0x11);

#undef CSTR_CUR_COMMAND
    }

}
#endif /* CIUT_ENABLED */



/*****************************************************************************/

/**
 * \brief ignore any data until reach to the header of UBX packet
 * \param buffer_in: the buffer contains received packets
 * \param sz_in: the byte size of the received packets
 * \param sz_processed: the bytes size processed in the buffer_in
 * \param sz_needed_in: the bytes size of data need to append to buffer_in
 *
 * \return <0 fatal error, user should kill this connection;
 *         =2 the packet illegal, the caller should check if sz_out > 0 and send back the response in buffer_out
 *         =1 need more data, the byte size need data is stored in sz_needed;
 *         =0 on successs, the variable sz_processed return processed data byte size
 */
int
ublox_pkt_nexthdr_ubx(uint8_t * buffer_in, size_t sz_in, size_t * sz_processed, size_t * sz_needed_in)
{
    uint8_t *p;
    uint8_t *p_end;

    p = buffer_in;
    p_end = buffer_in + sz_in;
    for (; p < p_end;) {
        p = strchr(p, 0xB5);
        if ((p >= p_end) || (p == 0)) {
            *sz_processed = sz_in;
            *sz_needed_in = UBLOX_PKT_LENGTH_HDR;
            assert(*sz_processed <= sz_in);
            return 1;
        }
        if (p + 1 >= p_end) {
            assert (p + 1 == p_end);
            *sz_processed = p - buffer_in;
            *sz_needed_in = UBLOX_PKT_LENGTH_HDR - (sz_in - *sz_processed);
            assert ((sz_in - *sz_processed) == 1);
            assert(*sz_processed <= sz_in);
            return 1;
        }
        if (*(p+1) != 0x62) {
            // search again
            p ++;
            continue;
        }
        *sz_processed = p - buffer_in;
        *sz_needed_in = 0;
        assert(*sz_processed <= sz_in);
       return 0;
    }
    assert(*sz_processed <= sz_in);
    return -1;
}

// return the expected pkt size
size_t
ublox_pkt_expected_size(uint8_t * buffer_in, size_t sz_in)
{
    // check header
    if (buffer_in[0] != 0xB5) {
        return 0;
    }
    if (buffer_in[1] != 0x62) {
        return 1;
    }
    // check class,id
    size_t sz = 8+UBLOX_PKG_LENGTH(buffer_in);
    switch (UBLOX_CLASS_ID(buffer_in[2], buffer_in[3])) {
    case UBX_MON_VER: break;
    case UBX_MON_HW:  sz=8+68; break;
    case UBX_MON_HW2: sz=8+28; break;
    case UBX_MON_RXR: sz=8+1; break;
    case UBX_ACK_ACK: sz=8+2; break;
    case UBX_ACK_NAK: sz=8+2; break;
    case UBX_CFG_MSG: sz=8+2; break;
    case UBX_RXM_RAW:
        sz = 8 + 8 + buffer_in[6+6];
        break;
    case UBX_RXM_SFRB: sz=8+42; break;
    case UBX_DBG_SET:  sz=8+16; break;
    }

    return sz;
}


static char * ublox_val2cstr_gnss(int type)
{
    switch (type) {
        case 0: return "GPS";
        case 1: return "SBS";
        case 2: return "GAL";
        case 3: return "CMP"; // beidou
        case 5: return "QZS";
        case 6: return "GLO";
    }
    return "UNKNOWN_GNSS";
}

/**
 * \brief read and verify the return packet from ublox module(TCP server)
 * \param buffer_in: the buffer contains received packets
 * \param sz_in: the byte size of the received packets
 * \param sz_processed: the bytes size processed in the buffer_in
 * \param sz_needed_in: the bytes size of data need to append to buffer_in
 *
 * \return <0 fatal error, user should kill this connection;
 *         =2 the packet illegal, the caller should check if sz_out > 0 and send back the response in buffer_out
 *         =1 need more data, the byte size need data is stored in sz_needed;
 *         =0 on successs, the variable sz_processed return processed data byte size
 */
int
ublox_cli_verify_tcp(uint8_t * buffer_in, size_t sz_in, size_t * sz_processed, size_t * sz_needed_in)
{
    size_t sz;
    int i;
    uint8_t *p;
    uint16_t classid;
    uint8_t status;
    uint16_t count;

    if (sz_processed) {
        *sz_processed = 0;
    }
    if (sz_needed_in) {
        *sz_needed_in = 0;
    }

    fprintf(stderr, "+++++++++++++++++++++++++++++++++++++++++++++\n");
    fprintf(stderr,"ublox_cli_verify() begin\n");
    // check the mininal size of packet
    if (NULL == buffer_in) {
        fprintf(stderr, "ublox warning: buffer NULL.\n");
        return -1;
    }

    if (sz_in < UBLOX_PKT_LENGTH_HDR) {
        assert (sz_needed_in);
        *sz_needed_in = UBLOX_PKT_LENGTH_HDR - sz_in;
        fprintf(stderr, "ublox warning: need more data, size=%" PRIuSZ ".\n", *sz_needed_in);
        return 1;
    }
    if (sz_in < UBLOX_PKT_LENGTH_MIN) {
        assert (sz_needed_in);
        *sz_needed_in = UBLOX_PKT_LENGTH_MIN + UBLOX_PKG_LENGTH(buffer_in) - sz_in;
        fprintf(stderr, "ublox warning: need more data, size=%" PRIuSZ ".\n", *sz_needed_in);
        return 1;
    }

    sz = ublox_pkt_expected_size(buffer_in, sz_in);
    if (sz > sz_in) {
        *sz_needed_in = sz - sz_in;
        return 1;
    }

    if (0 != ublox_pkt_verify(buffer_in, sz_in)) {
        fprintf(stderr, "ublox error: packet verify.\n");
        hex_dump_to_fd(STDERR_FILENO, buffer_in, sz_in);

        // try to skip the data
        *sz_processed = ublox_pkt_expected_size(buffer_in, sz_in);
        if (*sz_processed < 1) {
            *sz_processed = 1;
        }
        return 2;
    }

    assert (NULL != buffer_in);
    // read class
    classid = UBLOX_CLASS_ID(buffer_in[2], buffer_in[3]);
    // read count
    count = UBLOX_PKG_LENGTH(buffer_in);

    fprintf(stderr, "ublox info: received %s, value: 0x%04X\n", ublox_val2cstr_classid(buffer_in[2], buffer_in[3]), classid);

    p = buffer_in + 6;

    switch (classid) {
    case UBX_MON_VER:
    {
        char buf[40];
        assert(sizeof(buf)-1 >= 30);
        fprintf(stderr, "ublox firmware version:\n");
        if (count <= 0) {
            break;
        }

#define LEN_SEG 30
        buf[0] = 0;
        strncpy(buf, buffer_in + 6, LEN_SEG);
        buf[LEN_SEG] = 0;
        fprintf(stderr, "\tswVersion: %s\n", buf);
#undef LEN_SEG

#define LEN_SEG 10
        buf[0] = 0;
        strncpy(buf, buffer_in + 6 + 30, LEN_SEG);
        buf[LEN_SEG] = 0;
        fprintf(stderr, "\thwVersion: %s\n", buf);
#undef LEN_SEG

#define LEN_SEG 30
        buf[0] = 0;
        strncpy(buf, buffer_in + 6 + 30 + 10, LEN_SEG);
        buf[LEN_SEG] = 0;
        fprintf(stderr, "\tromVersion: %s\n", buf);
#undef LEN_SEG

#define LEN_SEG 30
        for (i = 0; 30 + 10 + 30 + i * 30 + 30 <= count; i ++) {
            buf[0] = 0;
            strncpy(buf, buffer_in + 6 + 30 + 10 + 30 + i * 30, LEN_SEG);
            buf[LEN_SEG] = 0;
            fprintf(stderr, "\textPackageVer[%d]: %s\n", i, buf);
        }
#undef LEN_SEG
    }
        break;


// little endian
#define U32_LE(p) ( \
    (uint32_t)(*((uint8_t *)(p) + 0)) \
    | ((uint32_t)(*((uint8_t *)(p) + 1)) << 8) \
    | ((uint32_t)(*((uint8_t *)(p) + 2)) << 16) \
    | ((uint32_t)(*((uint8_t *)(p) + 3)) << 24) \
    )
#define U16_LE(p) ( \
    (uint16_t)(*((uint8_t *)(p) + 0)) \
    | ((uint16_t)(*((uint8_t *)(p) + 1)) << 8) \
    )

    case UBX_MON_HW:
    {
        uint32_t val32;
        uint16_t val16;
        fprintf(stderr, "ublox hardware version:\n");

        assert(count == 68);

        val32 = U32_LE(p);
        fprintf(stderr, "\tpinSel: %08X\n", val32);
        p += 4;

        val32 = U32_LE(p);
        fprintf(stderr, "\tpinBank: %08X\n", val32);
        p += 4;

        val32 = U32_LE(p);
        fprintf(stderr, "\tpinDir: %08X\n", val32);
        p += 4;

        val32 = U32_LE(p);
        fprintf(stderr, "\tpinVal: %08X\n", val32);
        p += 4;

        val16 = U16_LE(p);
        fprintf(stderr, "\tnoisePerMS: %04X\n", val16);
        p += 2;

        val16 = U16_LE(p);
        fprintf(stderr, "\tagcCnt: %04X\n", val16);
        p += 2;

        fprintf(stderr, "\taStatus: %02X\n", *p);
        p += 1;

        fprintf(stderr, "\taPower: %02X\n", *p);
        p += 1;

        fprintf(stderr, "\tflags: %02X\n", *p);
        p += 1;

        fprintf(stderr, "\treserved1: %02X\n", *p);
        p += 1;

        val32 = U32_LE(p);
        fprintf(stderr, "\tusedMask: %08X\n", val32);
        p += 4;

        fprintf(stderr, "\tVP: sz=%d\n", 25);
        hex_dump_to_fd(STDERR_FILENO, p, 25);
        p += 25;

        fprintf(stderr, "\tjamInd: %02X\n", *p);
        p += 1;

        val16 = U16_LE(p);
        fprintf(stderr, "\treserved3: %04X\n", val16);
        p += 2;

        val32 = U32_LE(p);
        fprintf(stderr, "\tpinIrq: %08X\n", val32);
        p += 4;

        val32 = U32_LE(p);
        fprintf(stderr, "\tpullH: %08X\n", val32);
        p += 4;

        val32 = U32_LE(p);
        fprintf(stderr, "\tpullL: %08X\n", val32);
        p += 4;
    }
        break;

    case UBX_MON_HW2:
    {
        uint32_t val32;
        uint16_t val16;
        fprintf(stderr, "ublox hardware version 2:\n");

        assert(count == 68);

        fprintf(stderr, "\tofsI: %d\n", *((char *)p) );
        p += 1;

        fprintf(stderr, "\tmagI: %d\n", *p);
        p += 1;

        fprintf(stderr, "\tofsQ: %d\n", *((char *)p) );
        p += 1;

        fprintf(stderr, "\tmagQ: %d\n", *p);
        p += 1;

        fprintf(stderr, "\tcfgSource: %d\n", *p);
        p += 1;

        fprintf(stderr, "\treserved0: sz=%d\n", 3);
        hex_dump_to_fd(STDERR_FILENO, p, 3);
        p += 3;

        val32 = U32_LE(p);
        fprintf(stderr, "\tlowLevCfg: %08X\n", val32);
        p += 4;

        fprintf(stderr, "\treserved1: sz=%d\n", 8);
        hex_dump_to_fd(STDERR_FILENO, p, 8);
        p += 8;

        val32 = U32_LE(p);
        fprintf(stderr, "\tpostStatus: %08X\n", val32);
        p += 4;

        val32 = U32_LE(p);
        fprintf(stderr, "\treserved2: %08X\n", val32);
        p += 4;
    }
        break;

    case UBX_ACK_ACK:
    {
        assert(count == 2);

        fprintf(stderr, "ublox ACK:\n");

        fprintf(stderr, "\tclsID: %02X\n", *p);
        p += 1;

        fprintf(stderr, "\tmsgID: %02X\n", *p);
        p += 1;
    }
        break;

    case UBX_ACK_NAK:
    {
        assert(count == 2);

        fprintf(stderr, "ublox NAK:\n");

        fprintf(stderr, "\tclsID: %02X\n", *p);
        p += 1;

        fprintf(stderr, "\tmsgID: %02X\n", *p);
        p += 1;
    }
        break;

    case UBX_DBG_SET:
    {
        uint32_t val32;
        uint16_t val16;
        assert(count == 24);

        fprintf(stderr, "ublox !UBX DBG-SET:\n");

        val32 = U32_LE(p);
        fprintf(stderr, "\tX4_1: %08X\n", val32);
        p += 4;

        val32 = U32_LE(p);
        fprintf(stderr, "\tX4_2: %08X\n", val32);
        p += 4;

        val32 = U32_LE(p);
        fprintf(stderr, "\tX4_3: %08X\n", val32);
        p += 4;

        val16 = U16_LE(p);
        fprintf(stderr, "\tU2_1: %04X\n", val16);
        p += 2;

        fprintf(stderr, "\tclass: %02X\n", *p);
        p += 1;

        fprintf(stderr, "\tid: %02X\n", *p);
        p += 1;

    }
        break;

    case UBX_CFG_MSG:
    {
        assert(count >= 2);

        if (count == 2) {
            // query
            fprintf(stderr, "\t(type): Poll a message configuration\n");
        } else {
            fprintf(stderr, "\t(type): Set Message Rate(s)\n");
        }

        fprintf(stderr, "\tmsgClass: %02X\n", *p);
        p += 1;

        fprintf(stderr, "\tmsgID: %02X\n", *p);
        p += 1;

        for (; p - buffer_in - 6 < count; ) {
            fprintf(stderr, "\trate[%d]: %02X\n", *p);
            p += 1;
        }

    }
        break;

    case UBX_CFG_PRT:
    {
        uint32_t val32;
        uint16_t val16;
        uint8_t portid;
        assert(count >= 0);
        assert(count <= 20);

        if (count == 0) {
            // query
            fprintf(stderr, "\t(type): Polls the configuration of the used I/O Port\n");
            break;
        } else if (count == 1) {
            fprintf(stderr, "\t(type): Polls the configuration for one I/O Port\n");
        } else {
            fprintf(stderr, "\t(type): Gotten/Set Port Configuration\n");
        }

        // for UART
        // U1 portID(=1 or 2), U1 reserved0, X2 txReady, X4 mode, U4 baudRate, X2 inPortoMask, X2 outPortoMask, U2 reserved4, U2 reserved5
        // for USB
        // U1 portID(=3), U1 reserved0, X2 txReady, X4 reserved2, U4 reserved3, X2 inPortoMask, X2 outPortoMask, U2 reserved4, U2 reserved5
        // for SPI
        // U1 portID(=4), U1 reserved0, X2 txReady, X4 mode, U4 reserved3, X2 inPortoMask, X2 outPortoMask, U2 reserved4, U2 reserved5
        // for DDC
        // U1 portID(=0), U1 reserved0, X2 txReady, X4 mode, U4 reserved3, X2 inPortoMask, X2 outPortoMask, U2 reserved4, U2 reserved5

        portid = *p;
        fprintf(stderr, "\tPortID: %s (0x%02X)\n", ublox_val2cstr_portid(*p), *p);
        p += 1;

        if (count == 1) {
            break;
        }
        assert(count == 20);

        fprintf(stderr, "\treserved0: %02X\n", *p);
        p += 1;

        val16 = U16_LE(p);
        fprintf(stderr, "\ttxReady: %04X\n", val16);
        p += 2;

        if (portid == 3) {
            // usb
            val32 = U32_LE(p);
            fprintf(stderr, "\treserved2: %08X\n", val32);
            p += 4;
        } else {
            val32 = U32_LE(p);
            fprintf(stderr, "\tmode: %08X\n", val32);
            p += 4;
        }
        if (portid == 1 || portid == 2) {
            // UART
            val32 = U32_LE(p);
            fprintf(stderr, "\tbaudRate: %d(0x%08X)\n", val32, val32);
            p += 4;
        } else {
            val32 = U32_LE(p);
            fprintf(stderr, "\treserved3: %08X\n", val32);
            p += 4;
        }

        val16 = U16_LE(p);
        fprintf(stderr, "\tinPortoMask: %04X\n", val16);
        p += 2;

        val16 = U16_LE(p);
        fprintf(stderr, "\toutPortoMask: %04X\n", val16);
        p += 2;

        val16 = U16_LE(p);
        fprintf(stderr, "\treserved4: %04X\n", val16);
        p += 2;

        val16 = U16_LE(p);
        fprintf(stderr, "\treserved5: %04X\n", val16);
        p += 2;

    }
        break;

    case UBX_CFG_RATE:
    {
        uint16_t val16;
        assert(count >= 0);
        assert(count <= 6);

        if (count == 0) {
            // query
            fprintf(stderr, "\t(type): Poll Navigation/Measurement Rate Settings\n");
            break;
        } else {
            fprintf(stderr, "\t(type): Navigation/Measurement Rate Settings\n");
        }

        val16 = U16_LE(p);
        fprintf(stderr, "\tmeasRate: %d(0x%04X)\n", val16, val16);
        p += 2;

        val16 = U16_LE(p);
        fprintf(stderr, "\tnavRate: %d(0x%04X)\n", val16, val16);
        p += 2;

        val16 = U16_LE(p);
        fprintf(stderr, "\ttimeRef: %d(0x%04X)\n", val16, val16);
        p += 2;

    }
        break;


    case UBX_NAV_TIMEGPS:
    {
        uint32_t val32;
        int32_t vali32;
        uint16_t val16;
        int16_t vali16;
        assert(count == 16);

        val32 = U32_LE(p);
        fprintf(stderr, "\tiTOW: %08X\n", val32);
        p += 4;

        val32 = U32_LE(p);
        memmove(&vali32, &val32, sizeof(val32));
        fprintf(stderr, "\tfTOW: %d\n", vali32);
        p += 4;

        val16 = U16_LE(p);
        memmove(&vali16, &val16, sizeof(val16));
        fprintf(stderr, "\tweek: %d\n", vali16);
        p += 2;

        fprintf(stderr, "\tleapS: %d\n", *((char *)p));
        p += 1;

        fprintf(stderr, "\tvalid: %02X\n", *p);
        p += 1;

        val32 = U32_LE(p);
        fprintf(stderr, "\ttAcc: %08X\n", val32);
        p += 4;
    }
        break;

    case UBX_NAV_CLOCK:
    {
        uint32_t val32;
        int32_t vali32;
        uint16_t val16;
        int16_t vali16;
        assert(count == 20);

        val32 = U32_LE(p);
        fprintf(stderr, "\tiTOW: %08X\n", val32);
        p += 4;

        val32 = U32_LE(p);
        memmove(&vali32, &val32, sizeof(val32));
        fprintf(stderr, "\tclkB: %d\n", vali32);
        p += 4;

        val32 = U32_LE(p);
        memmove(&vali32, &val32, sizeof(val32));
        fprintf(stderr, "\tclkD: %d\n", vali32);
        p += 4;

        val32 = U32_LE(p);
        fprintf(stderr, "\ttAcc: %08X\n", val32);
        p += 4;

        val32 = U32_LE(p);
        fprintf(stderr, "\tfAcc: %08X\n", val32);
        p += 4;
    }
        break;

    case UBX_RXM_RAW:
    {
        // R4 IEEE 754 Single Precision
        // R8 IEEE 754 Double Precision
        int numSV;
        float  f4;
        double f8;

        uint32_t val32;
        int32_t vali32;
        uint16_t val16;
        int16_t vali16;

        // l4 iTOW
        val32 = U32_LE(p);
        memmove(&vali32, &val32, sizeof(val32));
        fprintf(stderr, "\tiTOW: %d\n", vali32);
        p += 4;

        // l2 week
        val16 = U16_LE(p);
        memmove(&vali16, &val16, sizeof(val16));
        fprintf(stderr, "\tweek: %d\n", vali16);
        p += 2;

        // U1 numSV
        numSV = *p;
        fprintf(stderr, "\tnumSV: %02X\n", *p);
        p += 1;

        // U1 reserved1
        fprintf(stderr, "\treserved1: %02X\n", *p);
        p += 1;

        // repeats: 24*numSV
        //   R8 cpMes
        //   R8 prMes
        //   R4 doMes
        //   U1 sv
        //   l1 mesQI
        //   l1 cno
        //   U1 lli
        for (i = 0; i < numSV; i ++) {
            memmove(&f8, p, sizeof(f8));
            fprintf(stderr, "\t[%d]\tcpMes: %f\n", i, f8);
            p += 8;

            memmove(&f8, p, sizeof(f8));
            fprintf(stderr, "\t\tprMes: %f\n", f8);
            p += 8;

            memmove(&f4, p, sizeof(f4));
            fprintf(stderr, "\t\tdoMes: %f\n", f4);
            p += 4;

            fprintf(stderr, "\t\tsv: %02X\n", *p);
            p += 1;

            fprintf(stderr, "\t\tmesQI: %d\n", *((char *)p));
            p += 1;

            fprintf(stderr, "\t\tcno: %d\n", *((char *)p));
            p += 1;

            fprintf(stderr, "\t\tlli: %02X\n", *p);
            p += 1;
        }
    }
        break;

    case UBX_RXM_SFRB:
    {
        uint32_t val32;

        // U1 chn
        fprintf(stderr, "\tchn: %02X\n", *p);
        p += 1;
        // U1 svid
        fprintf(stderr, "\tsvid: %02X\n", *p);
        p += 1;
        // X4[10] dwrd
        for(i = 0; i < 10; i ++) {
            val32 = U32_LE(p);
            fprintf(stderr, "\tdwrd[%d]: %d\n", i, val32);
            p += 4;
        }
    }
        break;

    case UBX_RXM_SFRBX: // ublox8
    {
        int numWords;
        uint32_t val32;

        // U1 gnssId
        fprintf(stderr, "\tgnssId: %02X\n", *p);
        p += 1;
        // U1 svId
        fprintf(stderr, "\tsvId: %02X\n", *p);
        p += 1;
        // U1 reserved1
        fprintf(stderr, "\treserved1: %02X\n", *p);
        p += 1;
        // U1 freqId
        fprintf(stderr, "\tfreqId: %02X\n", *p);
        p += 1;
        // U1 numWords
        numWords = *p;
        fprintf(stderr, "\tnumWords: %02X\n", *p);
        p += 1;
        // U1 reserved2
        fprintf(stderr, "\treserved2: %02X\n", *p);
        p += 1;
        // U1 version
        fprintf(stderr, "\tversion: %02X\n", *p);
        p += 1;
        // U1 reserved3
        fprintf(stderr, "\treserved3: %02X\n", *p);
        p += 1;

        // repeats: 4*numWords
        //   U4 dwrd
        for(i = 0; i < numWords; i ++) {
            val32 = U32_LE(p);
            fprintf(stderr, "\tdwrd[%d]: %08X\n", i, val32);
            p += 4;
        }
    }
        break;

    case UBX_RXM_RAWX: // ublox8
    {
        float  f4;
        double f8;
        uint16_t val16;
        int numMeas;

        // R8 rcvTow
        memmove(&f8, p, sizeof(f8));
        fprintf(stderr, "\trcvTow: %f\n", f8);
        p += 8;

        // U2 week
        val16 = U16_LE(p);
        fprintf(stderr, "\tweek: %d\n", val16);
        p += 2;

        // l1 leapS
        fprintf(stderr, "\tleapS: %d\n", *((char *)p));
        p += 1;

        // U1 numMeas
        numMeas = *p;
        fprintf(stderr, "\tnumMeas: %02X\n", *p);
        p += 1;

        // X1 recStat
        fprintf(stderr, "\trecStat: %02X\n", *p);
        p += 1;

        // U1[3] reserved1
        p += 3;

        // repeats: 32*numMeas
        //   R8 prMes
        //   R8 cpMes
        //   R4 doMes
        //   U1 gnssId
        //   U1 svId
        //   U1 reserved2
        //   U1 freqId
        //   U2 locktime
        //   U1 cno
        //   X1 prStdev
        //   X1 cpStdev
        //   X1 doStdev
        //   X1 trkStat
        //   U1 reserved3
        for (i = 0; i < numMeas; i ++) {
            memmove(&f8, p, sizeof(f8));
            fprintf(stderr, "\t[%d]\tprMes: %f\n", i, f8);
            p += 8;

            memmove(&f8, p, sizeof(f8));
            fprintf(stderr, "\t\tcpMes: %f\n", f8);
            p += 8;

            memmove(&f4, p, sizeof(f4));
            fprintf(stderr, "\t\tdoMes: %f\n", f4);
            p += 4;

            fprintf(stderr, "\t\tgnssId: %02X\n", *p);
            p += 1;

            fprintf(stderr, "\t\tsvId: %02X\n", *p);
            p += 1;

            p += 1;

            fprintf(stderr, "\t\tfreqId: %02X\n", *p);
            p += 1;

            val16 = U16_LE(p);
            fprintf(stderr, "\t\tlocktime: %04X\n", val16);
            p += 2;

            fprintf(stderr, "\t\tcno: %02X\n", *p);
            p += 1;

            fprintf(stderr, "\t\tprStdev: %02X\n", *p);
            p += 1;

            fprintf(stderr, "\t\tcpStdev: %02X\n", *p);
            p += 1;

            fprintf(stderr, "\t\tdoStdev: %02X\n", *p);
            p += 1;

            fprintf(stderr, "\t\ttrkStat: %02X\n", *p);
            p += 1;

            fprintf(stderr, "\t\treserved3: %02X\n", *p);
            p += 1;

        }

    }
        break;

    case UBX_TRK_D5:
    {
        float  f4;
        double f8;
        uint16_t val16;

        int len;
        int type;
        type = *p;

        fprintf(stderr, "\ttype: %d\n", type);
        switch(type) {
        case 3: p += 80; len = 56; break;
        case 6: p += 80; len = 64; break; // ublox7
        default: p += 72; len = 56; break;
        }
        for (i = 0; p < buffer_in + sz_in - 2; i ++, p += len) {
            memmove(&f8, p, sizeof(f8));
            fprintf(stderr, "\t[%d]\tts: %f\n", i, f8); // transmission time

            memmove(&f8, p+8, sizeof(f8));
            fprintf(stderr, "\t\tadr: %f\n", f8);
            memmove(&f4, p+16, sizeof(f4));
            fprintf(stderr, "\t\tdop: %f\n", f4);

            val16 = U16_LE(p+32);
            fprintf(stderr, "\t\tsnr: %04X\n", val16);

            // quality indicator
            fprintf(stderr, "\t\tqi=%02X\n", (*(p+41)) & 0x07);

#define MINPRNSBS   120                 /* min satellite PRN number of SBAS */
            if (type == 6) {
                fprintf(stderr, "\t\tgnssId=%s\n", ublox_val2cstr_gnss(*(p+56)));
                fprintf(stderr, "\t\tsvId=%d\n", (*(p+57)));
                fprintf(stderr, "\t\tfreqId=%d\n", (*(p+59)));
            } else {
                int svId;
                svId = *(p+34);
                fprintf(stderr, "\t\tgnssId=%s\n", ublox_val2cstr_gnss((svId<MINPRNSBS)?0/*GPS*/:1/*SBS*/));
                fprintf(stderr, "\t\tsvId=%d\n", (*(p+34)));
            }
            fprintf(stderr, "\t\tflags=%02X\n", (*(p+54)));

        }
    }
        break;

    case UBX_TRK_MEAS:
    {
        float  f4;
        double f8;
        uint16_t val16;

        int len;
        int nch; // number of channel
        nch = *(p + 2);

        fprintf(stderr, "\tnch: %d\n", nch);

        len = 56;
        p += 104;
        for (i = 0; p < buffer_in + sz_in - 2; i ++, p += len) {
            fprintf(stderr, "\t[%d]\tgnss=%s\n", i, ublox_val2cstr_gnss(*(p+4)));

            /* quality indicator (0:idle,1:search,2:aquired,3:unusable, */
            /*                    4:code lock,5,6,7:code/carrier lock) */
            fprintf(stderr, "\t\tqi=%02X\n", (*(p+1)));

            fprintf(stderr, "\t\tprn=%02X\n", (*(p+5)));

            memmove(&f8, p+24, sizeof(f8));
            fprintf(stderr, "\t\tts: %f\n", f8); // transmission time

            fprintf(stderr, "\t\tfrq=%02X\n", (*(p+7)));  // frequency
            fprintf(stderr, "\t\tflag=%02X\n", (*(p+8))); // tracking status
            fprintf(stderr, "\t\tlock1=%02X\n", (*(p+16))); // code lock count
            fprintf(stderr, "\t\tlock2=%02X\n", (*(p+17))); // phase lock count

            val16 = U16_LE(p + 20);
            fprintf(stderr, "\t\tsnr: %04X\n", val16);

            memmove(&f8, p+32, sizeof(f8));
            fprintf(stderr, "\t\tadr: %f\n", f8); // transmission time

            memmove(&f4, p+40, sizeof(f4));
            fprintf(stderr, "\t\tdop: %f\n", f4);
        }
    }
        break;

    case UBX_TRK_SFRBX:
    {
        int type;
        type = *(p+1);
        fprintf(stderr, "\ttype: %d\n", type);
        fprintf(stderr, "\tprn=%02X\n", (*(p+2)));

    }
        break;

    default:
        sz = ublox_pkt_expected_size(buffer_in, sz_in);
        if (sz < 1) {
            sz = 1;
        }
        if (sz > sz_in) {
            sz = sz_in;
        }
        *sz_processed = sz;
        assert(*sz_processed <= sz_in);

        fprintf(stderr, "ublox error: unsupport command in packet: classid=%s(0x%04X)\n", ublox_val2cstr_classid(buffer_in[2], buffer_in[3]), classid);
        hex_dump_to_fd(STDERR_FILENO, buffer_in, sz_in);

        return 2;
        break;
    }
    fprintf(stderr, "+++++++++++++++++++++++++++++++++++++++++++++\n");

    assert (NULL != sz_processed);
    *sz_processed = UBLOX_PKT_LENGTH_MIN + count;
    assert(*sz_processed <= sz_in);

    return 0;
}


#if defined(CIUT_ENABLED) && (CIUT_ENABLED == 1)
#include <ciut.h>

TEST_CASE( .name="test ublox inner function", .description="Test ublox inner functions." ) {

    SECTION("test ublox") {
        REQUIRE(0 == 0);
        CIUT_LOG ("ublox? %d", 0);
    }
}
#endif /* CIUT_ENABLED */


