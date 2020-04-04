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

#define TD(...)
#define TI(...)
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
        TE("Verify error: mini packet size.\n");
        return -1;
    }
    if (0xB5 != buffer[0]) {
        return -1;
    }
    if (0x62 != buffer[1]) {
        return -1;
    }
    count = UBLOX_PKG_LENGTH(buffer);
    ublox_pkt_checksum(buffer + 2, 4 + count, chksum);
    if ((chksum[0] == *(buffer + 6 + count)) && (chksum[1] == *(buffer + 6 + count + 1))) {
        return 0;
    }

    TE("Verify error: checksum. sz_buf=%ld,count=%d, expected=0x%02X%02X, got=0x%02X%02X\n", sz_buf, count, *(buffer + 6 + count), *(buffer + 6 + count + 1), chksum[0], chksum[1]);
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

    ublox_pkt_checksum(buffer + 2, 4, buffer + 6);

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
 * \brief fill the buffer with the 'UPD-DOWNL' packet
 * \param buffer:   the buffer to be filled
 * \param sz_buf:   the byte size of the buffer
 * \return <0 on fail, >0 the size of packet
 *
 * Download Data to Memory
 */
//!UBX UPD-DOWNL 000016C8 00000000 00216997 0000 02 10
ssize_t
ublox_pkt_create_upd_downl (uint8_t *buffer, size_t sz_buf, uint32_t startAddr, uint32_t flags, uint8_t *data, size_t len)
{
    ssize_t ret = 0;

    if (NULL == buffer) {
        return -1;
    }
    if (sz_buf < 8) {
        return -1;
    }
    if (sz_buf < 8 + 8 + len) {
        return -2;
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
    ret = 8 + len;
    buffer[4] = ret & 0xFF;
    buffer[5] = (ret >> 8) & 0xFF;

    // payload
    ret = 6;
    // little endian
    buffer[ret ++] = startAddr & 0xFF;
    buffer[ret ++] = (startAddr >> 8) & 0xFF;
    buffer[ret ++] = (startAddr >> 16) & 0xFF;
    buffer[ret ++] = (startAddr >> 24) & 0xFF;

    buffer[ret ++] = flags & 0xFF;
    buffer[ret ++] = (flags >> 8) & 0xFF;
    buffer[ret ++] = (flags >> 16) & 0xFF;
    buffer[ret ++] = (flags >> 24) & 0xFF;

    memmove(buffer + ret, data, len);
    ret += len;

    ublox_pkt_checksum(buffer + 2, ret - 2, buffer + ret);
    ret += 2;

    return ret;
}

/**
 * \brief fill the buffer with the 'CFG-BDS' packet
 * \param buffer:   the buffer to be filled
 * \param sz_buf:   the byte size of the buffer
 * \return <0 on fail, >0 the size of packet
 */
//!UBX CFG-BDS 00000000 00000000 0000001F FFFFFFFF 00000000 00000000
ssize_t
ublox_pkt_create_cfg_bds (uint8_t *buffer, size_t sz_buf, uint32_t u4_1, uint32_t u4_2, uint32_t u4_3_mask, uint32_t u4_4_mask, uint32_t u4_5, uint32_t u4_6)
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
    buffer[2] = 0x06;
    // ID
    buffer[3] = 0x4A;
    // length, little endian
    buffer[4] = 0x18; // 24 bytes
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

    buffer[ret ++] = u4_3_mask & 0xFF;
    buffer[ret ++] = (u4_3_mask >> 8) & 0xFF;
    buffer[ret ++] = (u4_3_mask >> 16) & 0xFF;
    buffer[ret ++] = (u4_3_mask >> 24) & 0xFF;

    buffer[ret ++] = u4_4_mask & 0xFF;
    buffer[ret ++] = (u4_4_mask >> 8) & 0xFF;
    buffer[ret ++] = (u4_4_mask >> 16) & 0xFF;
    buffer[ret ++] = (u4_4_mask >> 24) & 0xFF;

    buffer[ret ++] = u4_5 & 0xFF;
    buffer[ret ++] = (u4_5 >> 8) & 0xFF;
    buffer[ret ++] = (u4_5 >> 16) & 0xFF;
    buffer[ret ++] = (u4_5 >> 24) & 0xFF;

    buffer[ret ++] = u4_6 & 0xFF;
    buffer[ret ++] = (u4_6 >> 8) & 0xFF;
    buffer[ret ++] = (u4_6 >> 16) & 0xFF;
    buffer[ret ++] = (u4_6 >> 24) & 0xFF;

    ublox_pkt_checksum(buffer + 2, ret - 2, buffer + ret);
    ret += 2;
    assert(ret == 24 + 8);

    return ret;
}

#if defined(CIUT_ENABLED) && (CIUT_ENABLED == 1)
#include <ciut.h>

TEST_CASE( .name="ublox-bds", .description="Test ublox ublox_pkt_create_cfg_bds functions." ) {
    uint8_t buffer[30];
    SECTION("test ublox ublox_pkt_create_cfg_bds") {
        CIUT_LOG ("check ublox_pkt_create_cfg_bds %d", 0);

        REQUIRE(sizeof(buffer) > 24);
        //!HEX B5 62 06 4A 18 00 00 00 00 00 00 00 00 00 1F 00 00 00 FF FF FF FF 00 00 00 00 00 00 00 00 83 AC
        REQUIRE(24 + 8 == ublox_pkt_create_cfg_bds(buffer, sizeof(buffer), 0, 0, 31, 4294967295, 0, 0));
        CIUT_LOG("%d RAW buffer:", 0);
        hex_dump_to_fd(STDERR_FILENO, buffer, 24 + 8);
        REQUIRE(24 == UBLOX_PKG_LENGTH(buffer));
        REQUIRE(ublox_pkt_verify(buffer, UBLOX_PKG_LENGTH(buffer)+8) == 0);
        REQUIRE(*(buffer + UBLOX_PKG_LENGTH(buffer)+8-2) == 0x83);
        REQUIRE(*(buffer + UBLOX_PKG_LENGTH(buffer)+8-1) == 0xAC);
    }
}
#endif /* CIUT_ENABLED */


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

    TD("ublox_pkt_create_set_cfgmsg(class=%d, id=%d, reats[]=%d,%d,%d,%d,%d,%d\n", class, id, rates[0], rates[1], rates[2], rates[3], rates[4], rates[5]);

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

/**
 * \brief fill the buffer with the 'UBX-CFG-PRT' packet
 * \param buffer:   the buffer to be filled
 * \param sz_buf:   the byte size of the buffer
 * \param port_id:  the port id, 0xFF - void, 0 - I2C, 1 - UART1, 2 - UART2, 3 - USB, 4 - SPI
 * \param txReady:  
 * \param mode:     
 * \param baudRate: 
 * \param inPortoMask: 
 * \param outPortoMask: 
 * \return <0 on fail, >0 the size of packet
 */
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
 * \brief fill the buffer with the 'UBX-CFG-CFG' packet
 * \param buffer:   the buffer to be filled
 * \param sz_buf:   the byte size of the buffer
 * \param clear_mask: the clear mask
 * \param save_mask: 
 * \param load_mask: 
 * \param device_mask: 
 * \return <0 on fail, >0 the size of packet
 */
ssize_t
ublox_pkt_create_set_cfgcfg (uint8_t *buffer, size_t sz_buf, uint32_t clear_mask, uint32_t save_mask, uint32_t load_mask, uint8_t device_mask)
{
    ssize_t ret = 0;

    if (NULL == buffer) {
        return -1;
    }
    if (sz_buf < 8 + 13) {
        return -1;
    }
    assert (NULL != buffer);

    // header
    buffer[0] = 0xB5;
    buffer[1] = 0x62;
    // class
    buffer[2] = 0x06;
    // ID
    buffer[3] = 0x09;

    // length, little endian
    buffer[4] = 12;
    if (device_mask) {
        buffer[4] = 13;
    }
    buffer[5] = 0x00;

    // payload
    ret = 6;
    buffer[ret ++] = clear_mask & 0xFF;
    buffer[ret ++] = (clear_mask >> 8) & 0xFF;
    buffer[ret ++] = (clear_mask >> 16) & 0xFF;
    buffer[ret ++] = (clear_mask >> 24) & 0xFF;

    buffer[ret ++] = save_mask & 0xFF;
    buffer[ret ++] = (save_mask >> 8) & 0xFF;
    buffer[ret ++] = (save_mask >> 16) & 0xFF;
    buffer[ret ++] = (save_mask >> 24) & 0xFF;

    buffer[ret ++] = load_mask & 0xFF;
    buffer[ret ++] = (load_mask >> 8) & 0xFF;
    buffer[ret ++] = (load_mask >> 16) & 0xFF;
    buffer[ret ++] = (load_mask >> 24) & 0xFF;

    if (device_mask) {
        buffer[ret ++] = device_mask & 0xFF;
    }

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

ssize_t
ublox_pkt_create_set_cfg_gnss (uint8_t *buffer, size_t sz_buf, uint8_t msgVer, uint8_t numTrkChHw, uint8_t numTrkChUse, uint8_t numConfigBlocks, uint8_t *data)
{
    uint16_t val16;
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
    buffer[3] = 0x3E;

    // length, little endian
    val16 = 4 + 8 * numConfigBlocks;
    buffer[4] = val16 & 0xFF;
    buffer[5] = (val16 >> 8) & 0xFF;

    // payload
    ret = 6;

    buffer[ret ++] = msgVer;
    buffer[ret ++] = numTrkChHw;
    buffer[ret ++] = numTrkChUse;
    buffer[ret ++] = numConfigBlocks;
    memmove(buffer + ret, data, 8 * numConfigBlocks);
    ret += (8 * numConfigBlocks);

    ublox_pkt_checksum(buffer + 2, ret - 2, buffer + ret);
    ret += 2;

    return ret;
}


#if defined(CIUT_ENABLED) && (CIUT_ENABLED == 1)
#include <ciut.h>

TEST_CASE( .name="ublox-ext", .description="Test ublox ublox_pkt_create_get_version functions." ) {
    uint8_t buffer[30];
    SECTION("test ublox ublox_pkt_create_upd_downl") {
        CIUT_LOG ("check ublox_pkt_create_upd_downl %d", 0);

        REQUIRE(sizeof(buffer) > 24);
        //# RAW  b5 62 09 01 10 00 c8 16 00 00 00 00 00 00 97 69 21 00 00 00 02 10 2b 22
        char hex_1[] = {0x97, 0x69, 0x21, 0x00, 0x00, 0x00, 0x02, 0x10};
        REQUIRE(24 == ublox_pkt_create_upd_downl(buffer, sizeof(buffer), 0x000016C8, 0x00000000, hex_1, sizeof(hex_1)));
        CIUT_LOG("%d RAW buffer:", 0);
        hex_dump_to_fd(STDERR_FILENO, buffer, 24);
        REQUIRE(16 == UBLOX_PKG_LENGTH(buffer));
        REQUIRE(ublox_pkt_verify(buffer, 24) == 0);
        REQUIRE(*(buffer + 24-2) == 0x2B);
        REQUIRE(*(buffer + 24-1) == 0x22);

        //# SFRB b5 62 09 01 10 00 0c 19 00 00 00 00 00 00 83 69 21 00 00 00 02 11 5f f0
        char hex_2[] = {0x83, 0x69, 0x21, 0x00, 0x00, 0x00, 0x02, 0x11};
        REQUIRE(24 == ublox_pkt_create_upd_downl(buffer, sizeof(buffer), 0x0000190C, 0x00000000, hex_2, sizeof(hex_2)));
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
        CIUT_LOG("got (0x%08X 0x%08X 0x%08X 0x%04X 0x%02X 0x%02X) from '%s'\n", u4_1, u4_2, u4_3, u2_1, class, id, buf + sizeof(CSTR_CUR_COMMAND));
        REQUIRE(u4_1 == 0x000016C8);
        REQUIRE(u4_2 == 0x00000000);
        REQUIRE(u4_3 == 0x00216997);
        REQUIRE(u2_1 == 0x0000);
        REQUIRE(class == 0x02);
        REQUIRE(id == 0x10);

        strcpy(buf, "!UBX DBG-SET 0000190C 00000000 00216983 0000 02 11");
        sscanf(buf + sizeof(CSTR_CUR_COMMAND), "%x %X %X %X %X %X", &u4_1, &u4_2, &u4_3, &u2_1, &class, &id);
        CIUT_LOG("got (0x%08X 0x%08X 0x%08X 0x%04X 0x%02X 0x%02X) from '%s'\n", u4_1, u4_2, u4_3, u2_1, class, id, buf + sizeof(CSTR_CUR_COMMAND));
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
    case UBX_UPD_DOWNL: sz=8+16; break;
    }

    return sz;
}


static char * ublox_val2cstr_gnss(int gnss)
{
    switch (gnss) {
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

    TD("+++++++++++++++++++++++++++++++++++++++++++++\n");
    TD("ublox_cli_verify() begin\n");
    // check the mininal size of packet
    if (NULL == buffer_in) {
        TE("ublox warning: buffer NULL.\n");
        return -1;
    }

    if (sz_in < UBLOX_PKT_LENGTH_HDR) {
        assert (sz_needed_in);
        *sz_needed_in = UBLOX_PKT_LENGTH_HDR - sz_in;
        TI("ublox warning: need more data, size=%" PRIuSZ ".\n", *sz_needed_in);
        return 1;
    }
    if (sz_in < UBLOX_PKT_LENGTH_MIN) {
        assert (sz_needed_in);
        *sz_needed_in = UBLOX_PKT_LENGTH_MIN + UBLOX_PKG_LENGTH(buffer_in) - sz_in;
        TI("ublox warning: need more data, size=%" PRIuSZ ".\n", *sz_needed_in);
        return 1;
    }

    sz = ublox_pkt_expected_size(buffer_in, sz_in);
    if (sz > sz_in) {
        *sz_needed_in = sz - sz_in;
        return 1;
    }

    if (0 != ublox_pkt_verify(buffer_in, sz_in)) {
        TE("ublox error: packet verify.\n");
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

    TD("ublox info: received %s, value: 0x%04X\n", val2cstr_ublox_classid(buffer_in[2], buffer_in[3]), classid);

    p = buffer_in + 6;

    fprintf(stdout, "ublox %s:\n", val2cstr_ublox_classid(buffer_in[2], buffer_in[3]));
    switch (classid) {
    case UBX_MON_VER:
    {
        char buf[40];
        assert(sizeof(buf)-1 >= 30);
        //fprintf(stdout, "ublox firmware version:\n");

        if (count == 0) {
            // query
            fprintf(stdout, "\t(type): Poll Receiver/Software Version\n");
            break;
        } else {
            fprintf(stdout, "\t(type): Receiver/Software Version\n");
        }

#define LEN_SEG 30
        buf[0] = 0;
        strncpy(buf, buffer_in + 6, LEN_SEG);
        buf[LEN_SEG] = 0;
        fprintf(stdout, "\tswVersion: %s\n", buf);
#undef LEN_SEG

#define LEN_SEG 10
        buf[0] = 0;
        strncpy(buf, buffer_in + 6 + 30, LEN_SEG);
        buf[LEN_SEG] = 0;
        fprintf(stdout, "\thwVersion: %s\n", buf);
#undef LEN_SEG

        if (count >= 30 + 10 + 30) {
#define LEN_SEG 30
            buf[0] = 0;
            strncpy(buf, buffer_in + 6 + 30 + 10, LEN_SEG);
            buf[LEN_SEG] = 0;
            fprintf(stdout, "\tromVersion: %s\n", buf);
#undef LEN_SEG
        }

#define LEN_SEG 30
        for (i = 0; 30 + 10 + 30 + i * 30 + 30 <= count; i ++) {
            buf[0] = 0;
            strncpy(buf, buffer_in + 6 + 30 + 10 + 30 + i * 30, LEN_SEG);
            buf[LEN_SEG] = 0;
            fprintf(stdout, "\textPackageVer[%d]: %s\n", i, buf);
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
        //fprintf(stdout, "ublox hardware version:\n");

        if (count == 0) {
            // query
            fprintf(stdout, "\t(type): Poll Hardware Status\n");
            break;
        } else {
            fprintf(stdout, "\t(type): Hardware Status\n");
        }
        assert(count == 68);

        val32 = U32_LE(p);
        fprintf(stdout, "\tpinSel: %08X\n", val32);
        p += 4;

        val32 = U32_LE(p);
        fprintf(stdout, "\tpinBank: %08X\n", val32);
        p += 4;

        val32 = U32_LE(p);
        fprintf(stdout, "\tpinDir: %08X\n", val32);
        p += 4;

        val32 = U32_LE(p);
        fprintf(stdout, "\tpinVal: %08X\n", val32);
        p += 4;

        val16 = U16_LE(p);
        fprintf(stdout, "\tnoisePerMS: %04X\n", val16);
        p += 2;

        val16 = U16_LE(p);
        fprintf(stdout, "\tagcCnt: %04X\n", val16);
        p += 2;

        fprintf(stdout, "\taStatus: %02X\n", *p);
        p += 1;

        fprintf(stdout, "\taPower: %02X\n", *p);
        p += 1;

        fprintf(stdout, "\tflags: %02X\n", *p);
        p += 1;

        fprintf(stdout, "\treserved1: %02X\n", *p);
        p += 1;

        val32 = U32_LE(p);
        fprintf(stdout, "\tusedMask: %08X\n", val32);
        p += 4;

        fprintf(stdout, "\tVP: sz=%d\n", 25);
        hex_dump_to_fd(STDERR_FILENO, p, 25);
        p += 25;

        fprintf(stdout, "\tjamInd: %02X\n", *p);
        p += 1;

        val16 = U16_LE(p);
        fprintf(stdout, "\treserved3: %04X\n", val16);
        p += 2;

        val32 = U32_LE(p);
        fprintf(stdout, "\tpinIrq: %08X\n", val32);
        p += 4;

        val32 = U32_LE(p);
        fprintf(stdout, "\tpullH: %08X\n", val32);
        p += 4;

        val32 = U32_LE(p);
        fprintf(stdout, "\tpullL: %08X\n", val32);
        p += 4;
    }
        break;

    case UBX_MON_HW2:
    {
        uint32_t val32;
        //fprintf(stdout, "ublox hardware version 2:\n");

        if (count == 0) {
            // query
            fprintf(stdout, "\t(type): Poll Extended Hardware Status\n");
            break;
        } else {
            fprintf(stdout, "\t(type): Extended Hardware Status\n");
        }
        assert(count == 68);

        fprintf(stdout, "\tofsI: %d\n", *((char *)p) );
        p += 1;

        fprintf(stdout, "\tmagI: %d\n", *p);
        p += 1;

        fprintf(stdout, "\tofsQ: %d\n", *((char *)p) );
        p += 1;

        fprintf(stdout, "\tmagQ: %d\n", *p);
        p += 1;

        fprintf(stdout, "\tcfgSource: %d\n", *p);
        p += 1;

        fprintf(stdout, "\treserved0: sz=%d\n", 3);
        hex_dump_to_fd(STDERR_FILENO, p, 3);
        p += 3;

        val32 = U32_LE(p);
        fprintf(stdout, "\tlowLevCfg: %08X\n", val32);
        p += 4;

        fprintf(stdout, "\treserved1: sz=%d\n", 8);
        hex_dump_to_fd(STDERR_FILENO, p, 8);
        p += 8;

        val32 = U32_LE(p);
        fprintf(stdout, "\tpostStatus: %08X\n", val32);
        p += 4;

        val32 = U32_LE(p);
        fprintf(stdout, "\treserved2: %08X\n", val32);
        p += 4;
    }
        break;

    case UBX_ACK_ACK:
    {
        assert(count == 2);

        //fprintf(stdout, "ublox ACK:\n");

        fprintf(stdout, "\tclsID: %02X\n", *p);
        p += 1;

        fprintf(stdout, "\tmsgID: %02X\n", *p);
        p += 1;
    }
        break;

    case UBX_ACK_NAK:
    {
        assert(count == 2);

        //fprintf(stdout, "ublox NAK:\n");

        fprintf(stdout, "\tclsID: %02X\n", *p);
        p += 1;

        fprintf(stdout, "\tmsgID: %02X\n", *p);
        p += 1;
    }
        break;

    case UBX_UPD_DOWNL:
    {
        uint32_t val32;

        //fprintf(stdout, "ublox !UBX UPD-DOWNL:\n");

        val32 = U32_LE(p);
        fprintf(stdout, "\tStartAddr: %08X\n", val32);
        p += 4;

        val32 = U32_LE(p);
        fprintf(stdout, "\tFlags: %08X (%s)\n", val32, (val32?((val32 == 1)?"Download ACK":"Download NACK"):"Download"));
        p += 4;

        for (i = 0; p < buffer_in + 6 + count; i ++) {
            fprintf(stdout, "\tdata[%d]: %02X\n", i, *p);
            p += 1;
        }
    }
        break;

    case UBX_UPD_UPLOAD:
    {
        uint32_t val32;

        //fprintf(stdout, "ublox !UBX UPD-UPLOAD:\n");

        val32 = U32_LE(p);
        fprintf(stdout, "\tStartAddr: %08X\n", val32);
        p += 4;

        val32 = U32_LE(p);
        fprintf(stdout, "\tSize: %d\n", val32);
        p += 4;

        val32 = U32_LE(p);
        fprintf(stdout, "\tFlags: %08X (%s)\n", val32, (val32?((val32 == 1)?"Upload ACK":"Upload NACK"):"Upload"));
        p += 4;

        for (i = 0; p < buffer_in + 6 + count; i ++) {
            fprintf(stdout, "\tdata[%d]: %02X\n", i, *p);
            p += 1;
        }
    }
        break;

    case UBX_UPD_EXEC:
    {
        uint32_t val32;
        assert(count == 8);

        //fprintf(stdout, "ublox !UBX UPD-EXEC:\n");

        val32 = U32_LE(p);
        fprintf(stdout, "\tStartAddr: %08X\n", val32);
        p += 4;

        val32 = U32_LE(p);
        fprintf(stdout, "\tFlags: %08X (%s%s%s%s%s)\n"
                , val32
                , ((val32 & 0x01)?"Execution,":"Do not Execute")
                , ((val32 & 0x02)?"ACK,":"")
                , ((val32 & 0x04)?"NACK,":"")
                , ((val32 & 0x08)?"IRQs and FIQ enabled,":"IRQs and FIQ disabled")
                , ((val32 & 0x10)?"Reset after execution":"")
                );
        p += 4;
    }
        break;

    case UBX_UPD_MEMCPY:
    {
        uint32_t val32;
        assert(count == 16);

        //fprintf(stdout, "ublox !UBX UPD-MEMCPY:\n");

        val32 = U32_LE(p);
        fprintf(stdout, "\tStartAddr: %08X\n", val32);
        p += 4;

        val32 = U32_LE(p);
        fprintf(stdout, "\tDestAddr: %08X\n", val32);
        p += 4;

        val32 = U32_LE(p);
        fprintf(stdout, "\tSize: %08X\n", val32);
        p += 4;

        val32 = U32_LE(p);
        fprintf(stdout, "\tFlags: %08X (%s%s%s%s%s)\n"
                , val32
                , ((val32 & 0x01)?"Copy,":"Do not Copy")
                , ((val32 & 0x02)?"ACK,":"")
                , ((val32 & 0x04)?"NACK,":"")
                , ((val32 & 0x09)?"IRQs and FIQ enabled,":"IRQs and FIQ disabled")
                , ((val32 & 0x10)?"Reset after execution":"")
               );
        p += 4;
    }
        break;

    case UBX_UPD_SOS:
    {
        uint8_t val8;
        //fprintf(stdout, "ublox !UBX UPD-SOS:\n");

        if (count == 0) {
            // query
            fprintf(stdout, "\t(type): Poll Backup File Restore Status\n");
            break;
        }
        val8 = *p;
        switch (val8) {
        case 0:
            fprintf(stdout, "\t(type): Create Backup File in Flash\n");
            fprintf(stdout, "\tcmd: %02X\n", *p);
            p += 1;
            // reserved
            p += 3;
            break;
        case 1:
            fprintf(stdout, "\t(type): Clear Backup in Flash\n");
            fprintf(stdout, "\tcmd: %02X\n", *p);
            p += 1;
            // reserved
            p += 3;
            break;
        case 2:
            fprintf(stdout, "\t(type): Backup File Creation Acknowledge\n");
            fprintf(stdout, "\tcmd: %02X\n", *p);
            p += 1;
            // reserved
            p += 3;
            fprintf(stdout, "\tresponse: %02X(%s)\n", *p, (*p == 1)?"Acknowledged":"Not Acknowledged");
            p += 1;
            break;
        case 3:
            fprintf(stdout, "\t(type): System Restored from Backup\n");
            fprintf(stdout, "\tcmd: %02X\n", *p);
            p += 1;
            // reserved
            p += 3;
            fprintf(stdout, "\tresponse: %02X(%s)\n", *p, (*p > 0)?(*p==1?"Failed restoring from backup file":(*p==2?"Restored from backup file":(*p==3?"Not restored (no backup)":""))):"Unknown");
            p += 1;
            break;
        }
    }
        break;

    case UBX_CFG_BDS:
    {
        uint32_t val32;
        assert(count == 24);

        //fprintf(stdout, "ublox !UBX CFG-BDS:\n");

        val32 = U32_LE(p);
        fprintf(stdout, "\tX4_1: %08X\n", val32);
        p += 4;

        val32 = U32_LE(p);
        fprintf(stdout, "\tX4_2: %08X\n", val32);
        p += 4;

        val32 = U32_LE(p);
        fprintf(stdout, "\tX4_3: %08X\n", val32);
        p += 4;

        val32 = U32_LE(p);
        fprintf(stdout, "\tX4_4: %08X\n", val32);
        p += 4;

        val32 = U32_LE(p);
        fprintf(stdout, "\tX4_5: %08X\n", val32);
        p += 4;

        val32 = U32_LE(p);
        fprintf(stdout, "\tX4_6: %08X\n", val32);
        p += 4;
    }
        break;

    case UBX_CFG_GNSS:
    {
        uint32_t val32;
        int num;
        //fprintf(stdout, "ublox !UBX CFG-GNSS:\n");

        fprintf(stdout, "\tmsgVer: %02X\n", *p);
        p += 1;

        fprintf(stdout, "\tnumTrkChHw: %d\n", *p);
        p += 1;

        fprintf(stdout, "\tnumTrkChUse: %d\n", *p);
        p += 1;

        num = *p;
        fprintf(stdout, "\tnumConfigBlocks: %d\n", *p);
        p += 1;

        for (i = 0; i < num; i ++) {
            fprintf(stdout, "\t[%d]\tgnssId: %0d\n", i, *p);
            p += 1;
            fprintf(stdout, "\t\tresTrkCh: %d\n", *p);
            p += 1;
            fprintf(stdout, "\t\tmaxTrkCh: %d\n", *p);
            p += 1;
            fprintf(stdout, "\t\treserved1: %02X\n", *p);
            p += 1;

            val32 = U32_LE(p);
            fprintf(stdout, "\t\tflags: %08X (%s,sigCfgMask=%02X)\n"
                    , val32
                    , ((val32 & 0x01)?"Enabled":"Disabled")
                    , ((val32 >> 16) & 0xFF)
                   );
            p += 4;
        }
    }
        break;

    case UBX_CFG_MSG:
    {
        assert(count >= 2);

        //fprintf(stdout, "ublox !UBX CFG-MSG:\n");

        if (count == 2) {
            // query
            fprintf(stdout, "\t(type): Poll a message configuration\n");
        } else {
            fprintf(stdout, "\t(type): Set Message Rate(s)\n");
        }

        fprintf(stdout, "\tmsgClass: %02X\n", *p);
        p += 1;

        fprintf(stdout, "\tmsgID: %02X\n", *p);
        p += 1;
        fprintf(stdout, "\t(classid) %s\n", val2cstr_ublox_classid(*(p-2),*(p-1)));

        for (i = 0; p - buffer_in - 6 < count; i ++) {
            fprintf(stdout, "\tout[%d]: %02X (%s:%s)\n", i, *p, val2cstr_ublox_portid(i), (*p==0?"OFF":"ON"));
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

        //fprintf(stdout, "ublox !UBX CFG-PRT:\n");

        if (count == 0) {
            // query
            fprintf(stdout, "\t(type): Polls the configuration of the used I/O Port\n");
            break;
        } else if (count == 1) {
            fprintf(stdout, "\t(type): Polls the configuration for one I/O Port\n");
        } else {
            fprintf(stdout, "\t(type): Gotten/Set Port Configuration\n");
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
        fprintf(stdout, "\tPortID: %s (0x%02X)\n", val2cstr_ublox_portid(*p), *p);
        p += 1;

        if (count == 1) {
            break;
        }
        assert(count == 20);

        fprintf(stdout, "\treserved0: %02X\n", *p);
        p += 1;

        val16 = U16_LE(p);
        fprintf(stdout, "\ttxReady: %04X\n", val16);
        p += 2;

        if (portid == 3) {
            // usb
            val32 = U32_LE(p);
            fprintf(stdout, "\treserved2: %08X\n", val32);
            p += 4;
        } else {
            val32 = U32_LE(p);
            fprintf(stdout, "\tmode: %08X\n", val32);
            p += 4;
        }
        if (portid == 1 || portid == 2) {
            // UART
            val32 = U32_LE(p);
            fprintf(stdout, "\tbaudRate: %d(0x%08X)\n", val32, val32);
            p += 4;
        } else {
            val32 = U32_LE(p);
            fprintf(stdout, "\treserved3: %08X\n", val32);
            p += 4;
        }

        val16 = U16_LE(p);
        fprintf(stdout, "\tinPortoMask: %04X\n", val16);
        p += 2;

        val16 = U16_LE(p);
        fprintf(stdout, "\toutPortoMask: %04X\n", val16);
        p += 2;

        val16 = U16_LE(p);
        fprintf(stdout, "\treserved4: %04X\n", val16);
        p += 2;

        val16 = U16_LE(p);
        fprintf(stdout, "\treserved5: %04X\n", val16);
        p += 2;

    }
        break;

    case UBX_CFG_RATE:
    {
        uint16_t val16;
        assert(count >= 0);
        assert(count <= 6);

        //fprintf(stdout, "ublox !UBX CFG-RATE:\n");

        if (count == 0) {
            // query
            fprintf(stdout, "\t(type): Poll Navigation/Measurement Rate Settings\n");
            break;
        } else {
            fprintf(stdout, "\t(type): Navigation/Measurement Rate Settings\n");
        }

        val16 = U16_LE(p);
        fprintf(stdout, "\tmeasRate: %d(0x%04X)\n", val16, val16);
        p += 2;

        val16 = U16_LE(p);
        fprintf(stdout, "\tnavRate: %d(0x%04X)\n", val16, val16);
        p += 2;

        val16 = U16_LE(p);
        fprintf(stdout, "\ttimeRef: %d(0x%04X)\n", val16, val16);
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

        //fprintf(stdout, "ublox !UBX NAV-TIMEGPS:\n");

        val32 = U32_LE(p);
        fprintf(stdout, "\tiTOW: %08X\n", val32);
        p += 4;

        val32 = U32_LE(p);
        memmove(&vali32, &val32, sizeof(val32));
        fprintf(stdout, "\tfTOW: %d\n", vali32);
        p += 4;

        val16 = U16_LE(p);
        memmove(&vali16, &val16, sizeof(val16));
        fprintf(stdout, "\tweek: %d\n", vali16);
        p += 2;

        fprintf(stdout, "\tleapS: %d\n", *((char *)p));
        p += 1;

        fprintf(stdout, "\tvalid: %02X\n", *p);
        p += 1;

        val32 = U32_LE(p);
        fprintf(stdout, "\ttAcc: %08X\n", val32);
        p += 4;
    }
        break;

    case UBX_NAV_CLOCK:
    {
        uint32_t val32;
        int32_t vali32;
        assert(count == 20);
        //fprintf(stdout, "ublox !UBX NAV-CLOCK:\n");

        val32 = U32_LE(p);
        fprintf(stdout, "\tiTOW: %08X\n", val32);
        p += 4;

        val32 = U32_LE(p);
        memmove(&vali32, &val32, sizeof(val32));
        fprintf(stdout, "\tclkB: %d\n", vali32);
        p += 4;

        val32 = U32_LE(p);
        memmove(&vali32, &val32, sizeof(val32));
        fprintf(stdout, "\tclkD: %d\n", vali32);
        p += 4;

        val32 = U32_LE(p);
        fprintf(stdout, "\ttAcc: %08X\n", val32);
        p += 4;

        val32 = U32_LE(p);
        fprintf(stdout, "\tfAcc: %08X\n", val32);
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
        //fprintf(stdout, "ublox !UBX RXM-RAW:\n");

        // l4 iTOW
        val32 = U32_LE(p);
        memmove(&vali32, &val32, sizeof(val32));
        fprintf(stdout, "\tiTOW: %d\n", vali32);
        p += 4;

        // l2 week
        val16 = U16_LE(p);
        memmove(&vali16, &val16, sizeof(val16));
        fprintf(stdout, "\tweek: %d\n", vali16);
        p += 2;

        // U1 numSV
        numSV = *p;
        fprintf(stdout, "\tnumSV: %02X\n", *p);
        p += 1;

        // U1 reserved1
        fprintf(stdout, "\treserved1: %02X\n", *p);
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
            fprintf(stdout, "\t[%d]\tcpMes: %f\n", i, f8);
            p += 8;

            memmove(&f8, p, sizeof(f8));
            fprintf(stdout, "\t\tprMes: %f\n", f8);
            p += 8;

            memmove(&f4, p, sizeof(f4));
            fprintf(stdout, "\t\tdoMes: %f\n", f4);
            p += 4;

            fprintf(stdout, "\t\tsv: %02X\n", *p);
            p += 1;

            fprintf(stdout, "\t\tmesQI: %d\n", *((char *)p));
            p += 1;

            fprintf(stdout, "\t\tcno: %d\n", *((char *)p));
            p += 1;

            fprintf(stdout, "\t\tlli: %02X\n", *p);
            p += 1;
        }
    }
        break;

    case UBX_RXM_SFRB:
    {
        uint32_t val32;
        //fprintf(stdout, "ublox !UBX RXM-SFRB:\n");

        // U1 chn
        fprintf(stdout, "\tchn: %02X\n", *p);
        p += 1;
        // U1 svid
        fprintf(stdout, "\tsvid: %02X\n", *p);
        p += 1;
        // X4[10] dwrd
        for(i = 0; i < 10; i ++) {
            val32 = U32_LE(p);
            fprintf(stdout, "\tdwrd[%d]: %d\n", i, val32);
            p += 4;
        }
    }
        break;

    case UBX_RXM_SFRBX: // ublox8
    {
        int numWords;
        uint32_t val32;
        //fprintf(stdout, "ublox !UBX RXM-SFRBX:\n");

        // U1 gnssId
        fprintf(stdout, "\tgnssId: %02X\n", *p);
        p += 1;
        // U1 svId
        fprintf(stdout, "\tsvId: %02X\n", *p);
        p += 1;
        // U1 reserved1
        fprintf(stdout, "\treserved1: %02X\n", *p);
        p += 1;
        // U1 freqId
        fprintf(stdout, "\tfreqId: %02X\n", *p);
        p += 1;
        // U1 numWords
        numWords = *p;
        fprintf(stdout, "\tnumWords: %02X\n", *p);
        p += 1;
        // U1 reserved2
        fprintf(stdout, "\treserved2: %02X\n", *p);
        p += 1;
        // U1 version
        fprintf(stdout, "\tversion: %02X\n", *p);
        p += 1;
        // U1 reserved3
        fprintf(stdout, "\treserved3: %02X\n", *p);
        p += 1;

        // repeats: 4*numWords
        //   U4 dwrd
        for(i = 0; i < numWords; i ++) {
            val32 = U32_LE(p);
            fprintf(stdout, "\tdwrd[%d]: %08X\n", i, val32);
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
        //fprintf(stdout, "ublox !UBX RXM-RAWX:\n");

        // R8 rcvTow
        memmove(&f8, p, sizeof(f8));
        fprintf(stdout, "\trcvTow: %f\n", f8);
        p += 8;

        // U2 week
        val16 = U16_LE(p);
        fprintf(stdout, "\tweek: %d\n", val16);
        p += 2;

        // l1 leapS
        fprintf(stdout, "\tleapS: %d\n", *((char *)p));
        p += 1;

        // U1 numMeas
        numMeas = *p;
        fprintf(stdout, "\tnumMeas: %02X\n", *p);
        p += 1;

        // X1 recStat
        fprintf(stdout, "\trecStat: %02X\n", *p);
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
            fprintf(stdout, "\t[%d]\tprMes: %f\n", i, f8);
            p += 8;

            memmove(&f8, p, sizeof(f8));
            fprintf(stdout, "\t\tcpMes: %f\n", f8);
            p += 8;

            memmove(&f4, p, sizeof(f4));
            fprintf(stdout, "\t\tdoMes: %f\n", f4);
            p += 4;

            fprintf(stdout, "\t\tgnssId: %02X\n", *p);
            p += 1;

            fprintf(stdout, "\t\tsvId: %02X\n", *p);
            p += 1;

            p += 1;

            fprintf(stdout, "\t\tfreqId: %02X\n", *p);
            p += 1;

            val16 = U16_LE(p);
            fprintf(stdout, "\t\tlocktime: %04X\n", val16);
            p += 2;

            fprintf(stdout, "\t\tcno: %02X\n", *p);
            p += 1;

            fprintf(stdout, "\t\tprStdev: %02X\n", *p);
            p += 1;

            fprintf(stdout, "\t\tcpStdev: %02X\n", *p);
            p += 1;

            fprintf(stdout, "\t\tdoStdev: %02X\n", *p);
            p += 1;

            fprintf(stdout, "\t\ttrkStat: %02X\n", *p);
            p += 1;

            fprintf(stdout, "\t\treserved3: %02X\n", *p);
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
        //fprintf(stdout, "ublox !UBX TRK-D5:\n");

        fprintf(stdout, "\ttype: %d\n", type);
        switch(type) {
        case 3: p += 80; len = 56; break;
        case 6: p += 80; len = 64; break; // ublox7
        default: p += 72; len = 56; break;
        }
        for (i = 0; p < buffer_in + sz_in - 2; i ++, p += len) {
            memmove(&f8, p, sizeof(f8));
            fprintf(stdout, "\t[%d]\tts: %f\n", i, f8); // transmission time

            memmove(&f8, p+8, sizeof(f8));
            fprintf(stdout, "\t\tadr: %f\n", f8);
            memmove(&f4, p+16, sizeof(f4));
            fprintf(stdout, "\t\tdop: %f\n", f4);

            val16 = U16_LE(p+32);
            fprintf(stdout, "\t\tsnr: %04X\n", val16);

            // quality indicator
            fprintf(stdout, "\t\tqi=%02X\n", (*(p+41)) & 0x07);

#define MINPRNSBS   120                 /* min satellite PRN number of SBAS */
            if (type == 6) {
                fprintf(stdout, "\t\tgnssId=%s\n", ublox_val2cstr_gnss(*(p+56)));
                fprintf(stdout, "\t\tsvId=%d\n", (*(p+57)));
                fprintf(stdout, "\t\tfreqId=%d\n", (*(p+59)));
            } else {
                int svId;
                svId = *(p+34);
                fprintf(stdout, "\t\tgnssId=%s\n", ublox_val2cstr_gnss((svId<MINPRNSBS)?0/*GPS*/:1/*SBS*/));
                fprintf(stdout, "\t\tsvId=%d\n", (*(p+34)));
            }
            fprintf(stdout, "\t\tflags=%02X\n", (*(p+54)));

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

        //fprintf(stdout, "ublox !UBX TRK-MEAS:\n");

        val16 = U16_LE(p);
        fprintf(stdout, "\tunknown: %d\n", val16);
        p += 2;

        val16 = U16_LE(p);
        fprintf(stdout, "\tnch: %d (number of channels)\n", val16);
        p += 2;
        nch = val16;

        p += 100;
        len = 56;
        for (i = 0; p < buffer_in + sz_in - 2; i ++, p += len) {
            fprintf(stdout, "\t[%d]:\t# %d\n", i, *p);

            /* quality indicator (0:idle,1:search,2:aquired,3:unusable, */
            /*                    4:code lock,5,6,7:code/carrier lock) */
            fprintf(stdout, "\t\tqi=%02X\n", (*(p+1)));
            fprintf(stdout, "\t\tmesQI: %02X\n", *(p+2));
            fprintf(stdout, "\t\tgnss: %s\n", ublox_val2cstr_gnss(*(p+4)));
            fprintf(stdout, "\t\tsvid: %02X (satellite ID (PRN/slot number))\n", (*(p+5)));

            fprintf(stdout, "\t\tfcn: %02X (GLO frequency channel number+7)\n", (*(p+7)));
            fprintf(stdout, "\t\tstatus: %02X (tracking/lock status (bit3: half-cycle))\n", (*(p+8)));

            fprintf(stdout, "\t\tlock1: %02X (code lock count)\n", *(p+16));
            fprintf(stdout, "\t\tlock2: %02X (carrier lock count)\n", *(p+17));

            val16 = U16_LE(p + 20);
            fprintf(stdout, "\t\tcno: %04X (C/N0 (2^{-8} dBHz))\n", val16);

            memmove(&f8, p+24, sizeof(f8));
            fprintf(stdout, "\t\ttxTow: %f (transmission time in gps week (2^{-32} ms))\n", f8);

            memmove(&f8, p+32, sizeof(f8));
            fprintf(stdout, "\t\tadr: %f (accumulated Doppler range (2^{-32} cycle))\n", f8);

            memmove(&f4, p+40, sizeof(f4));
            fprintf(stdout, "\t\tdop: %f (Doppler frequency (2^{-32}x10 Hz))\n", f4);
        }
    }
        break;

    case UBX_TRK_SFRBX:
    {
        int gnss;

        //fprintf(stdout, "ublox !UBX TRK-SFRBX:\n");
        fprintf(stdout, "\tunknown: %02X\n", *p);
        p += 1;

        gnss = *p;
        fprintf(stdout, "\tgnss: %s\n", ublox_val2cstr_gnss(*p));
        p += 1;
        fprintf(stdout, "\tsvid: %02X (satellite ID (PRN/slot number))\n", *p);
        p += 1;

        p += 1;

        fprintf(stdout, "\tfcn: %02X (GLO frequency channel number+7)\n", (*p));
        p += 1;
        switch (gnss) {
        case 0: // "GPS"
            // X4[10] GPS nav GPS LNAV subframe (24 bits x 10)
            break;
        case 1: // "SBS"
            // X4[8] SBS nav SBAS message frame (226 bits)
            break;
        case 2: // "GAL"
            break;
        case 3: // beidou
            // X4[10] BDS nav BDS D1/D2 subframe (26 bits x 10)
            break;
        case 5: // "QZS"
            // X4[10] QZS nav QZSS LNAV subframe (24 bits x 10)
            break;
        case 6: // "GLO"
            // X4[4] GLO nav GLO nav string with hamming (85 bits)
            break;
        }

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

        TE("ublox error: unsupport command in packet: classid=%s(0x%04X)\n", val2cstr_ublox_classid(buffer_in[2], buffer_in[3]), classid);
        hex_dump_to_fd(STDERR_FILENO, buffer_in, sz_in);

        return 2;
        break;
    }
    TD("+++++++++++++++++++++++++++++++++++++++++++++\n");

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


