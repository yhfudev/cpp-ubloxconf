/**
 * \file    ubloxutils.h
 * \brief   support functions
 * \author  Yunhui Fu <yhfudev@gmail.com>
 * \version 1.0
 */
#ifndef _LIB_UBLOX_UTILS_H
#define _LIB_UBLOX_UTILS_H 1

#include <stdio.h>
#include <unistd.h> // STDERR_FILENO, usleep()

#include <stdlib.h>    /* size_t */
#include <sys/types.h> /* ssize_t pid_t */
#include <stdint.h> // uint8_t

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

typedef int (* pf_bsearch_cb_comp_t)(void *userdata, size_t idx, void * data_pin); /*"data_list[idx] - *data_pin"*/
int pf_bsearch_r (void *userdata, size_t num_data, pf_bsearch_cb_comp_t cb_comp, void *data_pinpoint, size_t *ret_idx);

/**
 * \brief parse the lines in the buffer
 * \param pos: the position in the file
 * \param buf: the buffer
 * \param size: the size of buffer
 * \param userdata: the pointer passed by the user
 *
 * \return 0 on successs, <0 on error
 *
 */
typedef int (* fread_line_cb_process_t)(off_t pos, char * buf, size_t size, void *userdata);

//int fread_lines (FILE *fp, void * userdata, int (* process)(off_t pos, char * buf, size_t size, void *userdata));
int read_file_lines(const char * fn_conf, void * userdata, int (* process)(off_t pos, char * buf, size_t size, void *userdata));

int cstr_strip(const char * orig, char * buf, size_t sz_buf);

ssize_t parse_hex_buf(char * cstr, size_t cslen, uint8_t * buf, size_t szbuf);

#ifndef NUM_ARRAY
#define NUM_ARRAY(a) (sizeof(a)/sizeof(a[0]))
#endif

#ifdef __cplusplus
}
#endif // __cplusplus

#endif /* _LIB_UBLOX_UTILS_H */
