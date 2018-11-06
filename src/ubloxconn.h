/**
 * \file    ubloxconn.h
 * \brief   UBlox module connection
 * \author  Yunhui Fu (yhfudev@gmail.com)
 * \version 1.0
 * \date    2018-11-06
 * \copyright GPL/BSD
 */

#ifndef UBLOX_CONN_H
#define UBLOX_CONN_H

#define DEBUG 1


#include <stdlib.h>    /* size_t */
#include <sys/types.h> /* ssize_t pid_t off64_t */
#include <stdint.h> // uint8_t

#ifdef _WIN32
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>

#ifndef fsync
#define fsync(fd) _commit(fd)
#endif
#ifndef u_int8_t
#define u_int8_t uint8_t
#define u_int16_t uint16_t
#define u_int32_t uint32_t
#define u_int64_t uint64_t
#endif

#else
#include <unistd.h> // STDERR_FILENO
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h> // inet_ntoa()
#endif

// printf formats
#include <inttypes.h> /* for PRIdPTR PRIiPTR PRIoPTR PRIuPTR PRIxPTR PRIXPTR, SCNdPTR SCNiPTR SCNoPTR SCNuPTR SCNxPTR */
#ifndef PRIuSZ
#ifdef __WIN32__                /* or whatever */
#define PRIiSZ "Id"
#define PRIuSZ "Iu"
#else
#define PRIiSZ "zd"
#define PRIuSZ "zu"
#define PRIxSZ "zx"
#define SCNxSZ "zx"
#endif
#define PRIiOFF PRIx64 /*"lld"*/
#define PRIuOFF PRIx64 /*"llu"*/
#endif // PRIuSZ



#ifdef __cplusplus
extern "C" {
#endif

#define UBLOX_PKT_LENGTH_HDR 6
#define UBLOX_PKT_LENGTH_MIN 8 /**< the mininal length of a UBLOX packet */

#define UBLOX_CLASS_NAV 0x01
#define UBLOX_CLASS_RXM 0x02
#define UBLOX_CLASS_INF 0x04
#define UBLOX_CLASS_ACK 0x05
#define UBLOX_CLASS_CFG 0x06
#define UBLOX_CLASS_MON 0x0A
#define UBLOX_CLASS_AID 0x0B
#define UBLOX_CLASS_ESF 0x10

int ublox_pkt_verify (uint8_t *buffer, size_t sz_buf);

// create a packet
ssize_t ublox_pkt_create_get_version (uint8_t *buffer, size_t sz_buf);
ssize_t ublox_pkt_create_get_hw (uint8_t *buffer, size_t sz_buf);
ssize_t ublox_pkt_create_get_hw2 (uint8_t *buffer, size_t sz_buf);

ssize_t ublox_pkt_create_unknown_msg1 (uint8_t *buffer, size_t sz_buf, uint32_t u4_1, uint32_t u4_2, uint32_t u4_3, uint16_t u2_1, uint8_t class, uint8_t id);

int ublox_pkt_nexthdr_ubx(uint8_t * buffer_in, size_t sz_in, size_t * sz_processed, size_t * sz_needed_in);
int ublox_cli_verify_tcp(uint8_t * buffer_in, size_t sz_in, size_t * sz_processed, size_t * sz_needed_in);


#ifdef __cplusplus
}
#endif

#endif /* UBLOX_CONN_H */


