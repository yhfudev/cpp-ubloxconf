/**
 * \file    ubloxcstr.h
 * \brief   UBlox C strings
 * \author  Yunhui Fu (yhfudev@gmail.com)
 * \version 1.0
 * \date    2018-11-14
 * \copyright GPL/BSD
 */

#ifndef UBLOX_CSTR_H
#define UBLOX_CSTR_H 1


#ifdef __cplusplus
extern "C" {
#endif

#ifndef NUM_ARRAY
#define NUM_ARRAY(a) (sizeof(a)/sizeof(a[0]))
#endif

const char * val2cstr_ublox_classid(uint16_t class_v, uint16_t id);
const char * val2cstr_ublox_portid(uint16_t port_id);
int cstr2val_ublox_classid(char * buf, size_t size, uint8_t * p_class, uint8_t *p_id);

ssize_t cstrlist2array_dec_val(const char *cstr_in, size_t len_cstr, char * buffer_out, size_t sz_bufout);
ssize_t cstrlist2array_hex_val(const char *cstr_in, size_t len_cstr, char * buffer_out, size_t sz_bufout);

ssize_t ublox_confline2bin_rtklibarg(char * buf_in, size_t sz_bufin, char * buf_out, size_t sz_bufout);
ssize_t ublox_confline2bin_hex(char * buf, size_t size, uint8_t * buf_out, size_t sz_bufout);

#ifdef __cplusplus
}
#endif

#endif /* UBLOX_CSTR_H */
