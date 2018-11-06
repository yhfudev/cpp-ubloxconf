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
#define UBX_MON_VER 0x0A04
#define UBX_MON_HW  0x0A09
#define UBX_MON_HW2 0x0A0B
#define UBX_MON_RXR 0x0A21

#define UBX_ACK_ACK 0x0501
#define UBX_ACK_NAK 0x0500

#define UBX_RXM_RAW  0x0210
#define UBX_RXM_SFRB 0x0211

#define UBX_UNK_MG1 0x0901


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

        UBLOX_V2S(UBX_UNK_MG1);
    }
    return "UNKNOWN_UBX_ID";
#undef UBLOX_V2S
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

#define UBLOX_PKG_LENGTH(p) ((unsigned int)((p)[4]) | (((unsigned int)((p)[5])) << 8))

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

    fprintf(stderr, "Verify error: checksum. sz=%d,len=%d, expected=0x%02X%02X, got=0x%02X%02X\n", sz_buf, count, *(buffer + 6 + count), *(buffer + 6 + count + 1), chksum[0], chksum[1]);
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
 * \brief fill the buffer with the 'unknown message 1' packet
 * \param buffer:   the buffer to be filled
 * \param sz_buf:   the byte size of the buffer
 * \return <0 on fail, >0 the size of packet
 */
//!UBX UNK-MG1 000016C8 00000000 00216997 0000 02 10
ssize_t
ublox_pkt_create_unknown_msg1 (uint8_t *buffer, size_t sz_buf, uint32_t u4_1, uint32_t u4_2, uint32_t u4_3, uint16_t u2_1, uint8_t class, uint8_t id)
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


#if defined(CIUT_ENABLED) && (CIUT_ENABLED == 1)
#include <ciut.h>

TEST_CASE( .name="ublox-ext", .description="Test ublox ublox_pkt_create_get_version functions." ) {
    uint8_t buffer[30];
    SECTION("test ublox ublox_pkt_create_unknown_msg1") {
        CIUT_LOG ("check ublox_pkt_create_unknown_msg1 %d", 0);

        REQUIRE(sizeof(buffer) > 24);
        //# RAW  b5 62 09 01 10 00 c8 16 00 00 00 00 00 00 97 69 21 00 00 00 02 10 2b 22
        REQUIRE(24 == ublox_pkt_create_unknown_msg1(buffer, sizeof(buffer), 0x000016C8, 0x00000000, 0x00216997, 0x0000, UBX_RXM_RAW >> 8, UBX_RXM_RAW & 0xFF));
        CIUT_LOG("%d RAW buffer:", 0);
        hex_dump_to_fd(STDERR_FILENO, buffer, 24);
        REQUIRE(16 == UBLOX_PKG_LENGTH(buffer));
        REQUIRE(ublox_pkt_verify(buffer, 24) == 0);
        REQUIRE(*(buffer + 24-2) == 0x2B);
        REQUIRE(*(buffer + 24-1) == 0x22);

        //# SFRB b5 62 09 01 10 00 0c 19 00 00 00 00 00 00 83 69 21 00 00 00 02 11 5f f0
        REQUIRE(24 == ublox_pkt_create_unknown_msg1(buffer, sizeof(buffer), 0x0000190C, 0x00000000, 0x00216983, 0x0000, UBX_RXM_SFRB >> 8, UBX_RXM_SFRB & 0xFF));
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

#define CSTR_CUR_COMMAND "!UBX UNK-MG1"
        char buf[256] = "!UBX UNK-MG1 000016C8 00000000 00216997 0000 02 10";
        sscanf(buf + sizeof(CSTR_CUR_COMMAND), "%X %X %X %X %X %X", &u4_1, &u4_2, &u4_3, &u2_1, &class, &id);
        fprintf(stderr, "got (0x%08X 0x%08X 0x%08X 0x%04X 0x%02X 0x%02X) from '%s'\n", u4_1, u4_2, u4_3, u2_1, class, id, buf + sizeof(CSTR_CUR_COMMAND));
        REQUIRE(u4_1 == 0x000016C8);
        REQUIRE(u4_2 == 0x00000000);
        REQUIRE(u4_3 == 0x00216997);
        REQUIRE(u2_1 == 0x0000);
        REQUIRE(class == 0x02);
        REQUIRE(id == 0x10);

        strcpy(buf, "!UBX UNK-MG1 0000190C 00000000 00216983 0000 02 11");
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
            return 1;
        }
        if (p + 1 >= p_end) {
            assert (p + 1 == p_end);
            *sz_processed = p - buffer_in;
            *sz_needed_in = UBLOX_PKT_LENGTH_HDR - (sz_in - *sz_processed);
            assert ((sz_in - *sz_processed) == 1);
            return 1;
        }
        if (*(p+1) != 0x62) {
            // search again
            p ++;
            continue;
        }
        *sz_processed = p - buffer_in;
        *sz_needed_in = 0;
        return 0;
    }
    return -1;
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
    if (0 != ublox_pkt_verify(buffer_in, sz_in)) {
        fprintf(stderr, "ublox error: packet verify.\n");
        hex_dump_to_fd(STDERR_FILENO, buffer_in, sz_in);
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
        fprintf(stderr, "ublox firmware version:\n");
        assert(sizeof(buf)-1 >= 30);

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
        uint8_t *p;
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

    case UBX_UNK_MG1:
    {
        uint32_t val32;
        uint16_t val16;
        assert(count == 24);

        fprintf(stderr, "ublox !UBX UNK-MG1:\n");

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

    default:
        fprintf(stderr, "ublox error: unsupport command in packet: classid=%s(0x%04X)\n", ublox_val2cstr_classid(buffer_in[2], buffer_in[3]), classid);
        return -1;
        break;
    }

    assert (NULL != sz_processed);
    *sz_processed = UBLOX_PKT_LENGTH_MIN + count;
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


