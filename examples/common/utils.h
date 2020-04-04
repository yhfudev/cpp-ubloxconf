/**
 * \file    utils.h
 * \brief   some misc functions
 * \author  Yunhui Fu (yhfudev@gmail.com)
 * \version 1.0
 * \date    2020-01-09
 * \copyright GPL/BSD
 */
#ifndef _MISC_HELP_FUNC_H
#define _MISC_HELP_FUNC_H 1

#include "osporting.h"

#ifdef __cplusplus
extern "C" {
#endif

void get_time_from_built(char *buf_ret, size_t sz_buf);

void setup_rand();

#ifdef __cplusplus
}
#endif

#endif // _MISC_HELP_FUNC_H

