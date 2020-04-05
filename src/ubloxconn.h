/**
 * \file    ubloxconn.h
 * \brief   UBlox module connection
 * \author  Yunhui Fu (yhfudev@gmail.com)
 * \version 1.0
 * \date    2018-11-06
 * \copyright GPL/BSD
 */

#ifndef UBLOX_CONN_H
#define UBLOX_CONN_H 1

#include "osporting.h"

#ifdef __cplusplus
extern "C" {
#endif

#define UBLOX_PKT_LENGTH_HDR 6
#define UBLOX_PKT_LENGTH_MIN 8 /**< the mininal length of a UBLOX packet */

#define UBLOX_CLASS_NAV 0x01
#define UBLOX_CLASS_RXM 0x02
#define UBLOX_CLASS_TRK 0x03
#define UBLOX_CLASS_INF 0x04
#define UBLOX_CLASS_ACK 0x05
#define UBLOX_CLASS_CFG 0x06
#define UBLOX_CLASS_UPD 0x09
#define UBLOX_CLASS_MON 0x0A
#define UBLOX_CLASS_AID 0x0B
#define UBLOX_CLASS_ESF 0x10
#define UBLOX_CLASS_MGA 0x13
#define UBLOX_CLASS_LOG 0x21
#define UBLOX_CLASS_SEC 0x27
#define UBLOX_CLASS_HNR 0x28

#define UBX_NAV_CLOCK   0x0122
#define UBX_NAV_PVT     0x0107
#define UBX_NAV_SOL     0x0106
#define UBX_NAV_STATUS  0x0103
#define UBX_NAV_SVINFO  0x0130
#define UBX_NAV_TIMEGPS 0x0120
#define UBX_NAV_VELNED  0x0112

#define UBX_RXM_RAW   0x0210
#define UBX_RXM_SFRB  0x0211
#define UBX_RXM_SFRBX 0x0213
#define UBX_RXM_RAWX  0x0215

#define UBX_TRK_D2    0x0306
#define UBX_TRK_D5    0x030A
#define UBX_TRK_MEAS  0x0310
#define UBX_TRK_SFRB  0x0302
#define UBX_TRK_SFRBX 0x030F

#define UBX_ACK_NAK  0x0500
#define UBX_ACK_ACK  0x0501

#define UBX_CFG_ANT 0x0613
#define UBX_CFG_BATCH 0x0693
#define UBX_CFG_BDS 0x064A
#define UBX_CFG_CFG 0x0609
#define UBX_CFG_DAT 0x0606
#define UBX_CFG_DGNSS 0x0670
#define UBX_CFG_DYNSEED 0x0685
#define UBX_CFG_EKF 0x0612
#define UBX_CFG_ESFGWT 0x0629
#define UBX_CFG_ESRC 0x0660
#define UBX_CFG_FIXSEED 0x0684
#define UBX_CFG_FXN  0x060E
#define UBX_CFG_GEOFENCE 0x0669
#define UBX_CFG_GNSS 0x063E
#define UBX_CFG_HNR  0x065C
#define UBX_CFG_INF  0x0602
#define UBX_CFG_ITFM 0x0639
#define UBX_CFG_LOGFILTER 0x0647
#define UBX_CFG_MSG  0x0601
#define UBX_CFG_NAV5 0x0624
#define UBX_CFG_NAVX5 0x0623
#define UBX_CFG_NMEA  0x0617
#define UBX_CFG_NVS   0x0622
#define UBX_CFG_ODO   0x061E
#define UBX_CFG_PM    0x0632
#define UBX_CFG_PM2   0x063B
#define UBX_CFG_PMS   0x0686
#define UBX_CFG_PRT   0x0600
#define UBX_CFG_PWR   0x0657
#define UBX_CFG_RATE  0x0608
#define UBX_CFG_RINV  0x0634
#define UBX_CFG_RST   0x0604
#define UBX_CFG_RXM   0x0611
#define UBX_CFG_SBAS  0x0616
#define UBX_CFG_SMGR  0x0662
#define UBX_CFG_TMODE 0x061D
#define UBX_CFG_TMODE2 0x063D
#define UBX_CFG_TMODE3 0x0671
#define UBX_CFG_TP    0x0607
#define UBX_CFG_TP5   0x0631
#define UBX_CFG_USB   0x061B

#define UBX_UPD_DOWNL  0x0901
#define UBX_UPD_EXEC   0x0903
#define UBX_UPD_MEMCPY 0x0904
#define UBX_UPD_SOS    0x0914 // ublox 8 protocol doc
#define UBX_UPD_UPLOAD 0x0902

#define UBX_MON_HW    0x0A09
#define UBX_MON_HW2   0x0A0B
#define UBX_MON_IO    0x0A02
#define UBX_MON_MSGPP 0x0A06
#define UBX_MON_RXBUF 0x0A07
#define UBX_MON_RXR   0x0A21
#define UBX_MON_TXBUF 0x0A08
#define UBX_MON_VER   0x0A04

#define UBX_TIM_TM2   0x0D03

// for use in CFG-MSG
#define UBX_NMEA_GxGGA 0xF000
#define UBX_NMEA_GxGLL 0xF001
#define UBX_NMEA_GxGSA 0xF002
#define UBX_NMEA_GxGSV 0xF003
#define UBX_NMEA_GxRMC 0xF004
#define UBX_NMEA_GxVTG 0xF005
#define UBX_NMEA_GxGRS 0xF006
#define UBX_NMEA_GxGST 0xF007
#define UBX_NMEA_GxZDA 0xF008
#define UBX_NMEA_GxGBS 0xF009
#define UBX_NMEA_GxDTM 0xF00A
#define UBX_NMEA_GxGNS 0xF00D
#define UBX_NMEA_GxTHS 0xF00E
#define UBX_NMEA_GxVLW 0xF00F

#define UBX_PUBX_POS  0xF100
#define UBX_PUBX_01   0xF101
#define UBX_PUBX_SV   0xF103
#define UBX_PUBX_TIME 0xF104
#define UBX_PUBX_POS2 0xF105
#define UBX_PUBX_POS3 0xF106

#define UBX_RTCM32_1005 0xF505
#define UBX_RTCM32_1074 0xF54A
#define UBX_RTCM32_1077 0xF54D
#define UBX_RTCM32_1084 0xF554
#define UBX_RTCM32_1087 0xF557
#define UBX_RTCM32_1124 0xF57C
#define UBX_RTCM32_1127 0xF57F
#define UBX_RTCM32_1230 0xF5E6
#define UBX_RTCM32_4072 0xF5FE

#define UBLOX_PKG_LENGTH(p) ((unsigned int)((p)[4]) | (((unsigned int)((p)[5])) << 8))

#define UBLOX_CLASS_ID(class,id) (((class) << 8) | (id))
#define UBLOX_2CLASS(class_id) (((class_id) & 0xFF00) >> 8)
#define UBLOX_2ID(class_id) ((class_id) & 0xFF)

void ublox_pkt_checksum(void *buffer, int length, uint8_t * out_buf);
int ublox_pkt_verify (uint8_t *buffer, size_t sz_buf);

// create a packet
ssize_t ublox_pkt_create_get_version (uint8_t *buffer, size_t sz_buf);
ssize_t ublox_pkt_create_get_hw (uint8_t *buffer, size_t sz_buf);
ssize_t ublox_pkt_create_get_hw2 (uint8_t *buffer, size_t sz_buf);

ssize_t ublox_pkt_create_set_cfgmsg (uint8_t *buffer, size_t sz_buf, uint8_t class_v, uint8_t id, uint8_t *rates, int num_rate);
ssize_t ublox_pkt_create_get_cfgprt (uint8_t *buffer, size_t sz_buf, uint8_t port_id);
ssize_t ublox_pkt_create_set_cfgprt (uint8_t *buffer, size_t sz_buf, uint8_t port_id, uint16_t txReady, uint32_t mode, uint32_t baudRate, uint16_t inPortoMask, uint16_t outPortoMask);
ssize_t ublox_pkt_create_set_cfg_gnss (uint8_t *buffer, size_t sz_buf, uint8_t msgVer, uint8_t numTrkChHw, uint8_t numTrkChUse, uint8_t numConfigBlocks, uint8_t *data);
ssize_t ublox_pkt_create_set_cfgcfg (uint8_t *buffer, size_t sz_buf, uint32_t clear_mask, uint32_t save_mask, uint32_t load_mask, uint8_t device_mask);

ssize_t ublox_pkt_create_get_cfgrate (uint8_t *buffer, size_t sz_buf);
ssize_t ublox_pkt_create_set_cfgrate (uint8_t *buffer, size_t sz_buf, uint16_t measRate, uint16_t navRate, uint16_t timeRef);

ssize_t ublox_pkt_create_upd_downl (uint8_t *buffer, size_t sz_buf, uint32_t startAddr, uint32_t flags, uint8_t *data, size_t len);
ssize_t ublox_pkt_create_cfg_bds (uint8_t *buffer, size_t sz_buf, uint32_t u4_1, uint32_t u4_2, uint32_t u4_3_mask, uint32_t u4_4_mask, uint32_t u4_5, uint32_t u4_6);

int ublox_pkt_nexthdr_ubx(uint8_t * buffer_in, size_t sz_in, size_t * sz_processed, size_t * sz_needed_in);
int ublox_cli_verify_tcp(uint8_t * buffer_in, size_t sz_in, size_t * sz_processed, size_t * sz_needed_in);
int ublox_process_buffer_data(uint8_t * buffer_in, size_t sz_in, size_t * psz_processed, size_t * psz_needed_in);

#ifdef __cplusplus
}
#endif

#endif /* UBLOX_CONN_H */


