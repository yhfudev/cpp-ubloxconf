/**
 * \file    ubloxconf.c
 * \brief   UBlox module configuration
 * \author  Yunhui Fu (yhfudev@gmail.com)
 * \version 1.0
 * \date    2018-11-06
 * \copyright GPL/BSD
 */

#define VER_MAJOR 0
#define VER_MINOR 1
#define VER_MOD   0

#define UBLOX_PORT_DEFAULT 23


#include <stdio.h>
#include <stdlib.h> // exit()
#include <getopt.h>
#include <libgen.h> // basename()
#include <assert.h>

#include <uv.h>

#include "utils.h"
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

#ifndef NUM_ARRAY
#define NUM_ARRAY(a) (sizeof(a)/sizeof(a[0]))
#endif

typedef struct _ubloxdata_client_t {
    const char * fn_execute; /**< the file name of execute file */

    struct sockaddr_in addr_tcp;  /**< thep socket addr for commands (TCP) */
    uv_tcp_t uvtcp;
    uv_connect_t connect;
    // TODO: a list of remote commands load from file?
    size_t num_requests; /**< the total number of requests sent */
    size_t num_responds; /**< the total number of responds received */
    time_t starttime;
    time_t timeout;

    size_t sz_data; /**< the lenght of data in the buffer */
    uint8_t buffer[UBLOX_PKT_LENGTH_MIN + 1200]; /**< the buffer to cache the received packets */
} ubloxdata_client_t;

ubloxdata_client_t g_ubxcli;
uv_loop_t * loop = NULL; /**< this have to be global variable, since it needs to access in on_xxxx() when service new connections */


/*****************************************************************************/
typedef struct {
    uv_write_t req;
    uv_buf_t buf;
} write_buf_t;

void
write_buf_free (write_buf_t *wr)
{
    free(wr->buf.base);
    free(wr);
}

void
alloc_buffer(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf)
{
    buf->base = malloc(suggested_size);
    buf->len = suggested_size;
}

/*****************************************************************************/

/**
 * \brief process response packet in the buffer of ubloxdata_client_t
 * \param ped: the ubxcli with buffer
 * \param stream: the libuv socket
 *
 * \return 0 on successs
 *
 * process the data in the buffer, until there's no more data can be processed in one round
 */
ssize_t
ubxcli_process_data (ubloxdata_client_t * ped, uv_stream_t *stream)
{
    uint8_t * buffer_in = NULL;
    size_t sz_in;
    size_t sz_processed;
    size_t sz_needed_in;

    //int flg_again = 0;
    int ret;

    assert (NULL != ped);
    assert (NULL != stream);

    fprintf(stderr,"tcp cli ubxcli_process_data() BEGIN\n");
    while(1) { // while (ped->sz_data > 0) {
        buffer_in = ped->buffer;
        sz_in = ped->sz_data;
        //flg_again = 0;
        sz_processed = 0;
        sz_needed_in = 0;

        fprintf(stderr,"tcp cli ubxcli_process_data() ped->sz_data=%" PRIuSZ "\n", ped->sz_data);
        assert (NULL != buffer_in);
        assert (sz_in >= 0);
        fprintf(stderr,"tcp cli ubxcli_process_data() call ublox_pkt_nexthdr_ubx\n");

        ret = ublox_pkt_nexthdr_ubx(buffer_in, sz_in, &sz_processed, &sz_needed_in);
        if (sz_processed > 0) {
            // remove the head of data of size sz_processed
            size_t sz_rest;
            assert (sz_processed <= ped->sz_data);
            sz_rest = ped->sz_data - sz_processed;
            if (sz_rest > 0) {
                memmove (ped->buffer, &(ped->buffer[sz_processed]), sz_rest);
            }
            ped->sz_data = sz_rest;
        }
        if (sz_needed_in > 0) {
            fprintf(stderr, "need more data: %" PRIuSZ "\n", sz_needed_in);
            break;
        }
        if (ret < 0) {
            break;
        } else if (ret == 0) {
            //flg_again = 1;

            ret = ublox_cli_verify_tcp(buffer_in, sz_in, &sz_processed, &sz_needed_in);

            if (sz_processed > 0) {
                // remove the head of data of size sz_processed
                size_t sz_rest;
                //assert (sz_processed <= ped->sz_data);
                if (sz_processed > ped->sz_data) {
                    sz_processed = ped->sz_data;
                }
                sz_rest = ped->sz_data - sz_processed;
                if (sz_rest > 0) {
                    memmove (ped->buffer, &(ped->buffer[sz_processed]), sz_rest);
                }
                ped->sz_data = sz_rest;
            }
            if (sz_needed_in > 0) {
                fprintf(stderr, "need more data: %" PRIuSZ "\n", sz_needed_in);
                break;
            }
            if (ret < 0) {
                break;
            } else if (ret == 0) {
                //flg_again = 1;
                if (g_ubxcli.timeout > 0) g_ubxcli.num_responds ++;
            } else if (ret == 1) {
                //flg_again = 1;
            } else if (ret == 2) {
                break;
            }

        } else if (ret == 1) {
            //flg_again = 1;

        } else if (ret == 2) {
            break;
        }

    }
    return 0;
}

/*****************************************************************************/

void
on_tcp_cli_close(uv_handle_t* handle)
{
    fprintf(stderr, "tcp cli closed.\n");
}

void
on_tcp_cli_read(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf)
{
    if(nread > 0) {
        // push the content to a center buffer for this TCP connection
        // and fetch the cmd to fetch appriate length of data as a packet
        // to process
        size_t sz_copy;
        size_t sz_processed;

        fprintf(stderr,"tcp cli read block, size=%" PRIiSZ ":\n", nread);
        hex_dump_to_fd(STDERR_FILENO, (opaque_t *)(buf->base), nread);

        fprintf(stderr, "tcp cli process data 1\n");
        ubxcli_process_data (&g_ubxcli, stream);
        sz_processed = 0;
        while (sz_processed < nread) {
            sz_copy = nread - sz_processed;
            assert (sizeof(g_ubxcli.buffer) >= g_ubxcli.sz_data);
            if (sz_copy + g_ubxcli.sz_data > sizeof(g_ubxcli.buffer)) {
                sz_copy = sizeof(g_ubxcli.buffer) - g_ubxcli.sz_data;
            }
            fprintf(stderr, "tcp cli push received data size=%" PRIuSZ ", processed=%" PRIuSZ "\n", sz_copy, sz_processed);
            if (sz_copy > 0) {
                memmove (g_ubxcli.buffer + g_ubxcli.sz_data, buf->base + sz_processed, sz_copy);
                sz_processed += sz_copy;
                g_ubxcli.sz_data += sz_copy;
            } else {
                // error? full?
                fprintf(stderr, "tcp cli no more received data to be push\n");
                break;
            }
            fprintf(stderr, "tcp cli process data 2\n");
            ubxcli_process_data (&g_ubxcli, stream);
        }
        if (sz_processed < nread) {
            // we're stalled here, because the content can't be processed by the function ubxcli_process_data()
            // error
            fprintf(stderr, "tcp cli data stalled\n");
            //uv_close((uv_handle_t *)stream, on_tcp_cli_close);
        }
    }
    if (nread == 0) {
        fprintf(stderr,"tcp cli read zero!\n");
    }
    if (nread < 0) {
        //we got an EOF
        fprintf(stderr,"tcp cli read EOF!\n");
        uv_close((uv_handle_t*)stream, on_tcp_cli_close);
    }

    free(buf->base);
    if (g_ubxcli.num_responds >= g_ubxcli.num_requests) {
        fprintf(stderr,"tcp cli received responses(%" PRIuSZ ") exceed requests(%" PRIuSZ ")!\n", g_ubxcli.num_responds, g_ubxcli.num_requests);
        uv_close((uv_handle_t*)stream, on_tcp_cli_close);
        raise(SIGINT); // send signal and handle by uv_signal_cb
    }
}

void
on_tcp_cli_write_end(uv_write_t* req, int status)
{
    if (status) {
        fprintf(stderr, "tcp cli write error %s.\n", uv_strerror(status));
        uv_close(req->handle, on_tcp_cli_close);
        return;
    } else {
        fprintf(stderr, "tcp cli write successfull.\n");
    }
    write_buf_free(req);
}

void
send_buffer(uv_stream_t* stream, uint8_t *buffer1, size_t szbuf)
{
    int r;
    write_buf_t * wbuf = NULL;

    wbuf = malloc (sizeof(*wbuf));
    assert (NULL != wbuf);
    memset(wbuf, 0, sizeof(*wbuf));
    alloc_buffer(NULL, szbuf, &(wbuf->buf));
    memmove (wbuf->buf.base, buffer1, szbuf);
    assert (NULL != wbuf->buf.base);
    assert (wbuf->buf.len >= szbuf);
    r = uv_write(wbuf, stream, &(wbuf->buf), 1, on_tcp_cli_write_end);
    if (r) {
        /* error */
        fprintf(stderr, "tcp cli error in write() %s\n", uv_strerror(r));
    }
}

#define STRCMP_STATIC(buf, static_str) strncmp(buf, static_str, sizeof(static_str)-1)

typedef struct _cstr_val_t {
    const char * cstr;
    uint8_t      val;
} cstr_val_t;

typedef struct _record_cstr_val_t{
    cstr_val_t * cstrval;
    size_t sz_val;
} record_cstr_val_t;

/*"data_list[idx] - *data_pin"*/
int
pf_bsearch_cb_comp_cstrval(void *userdata, size_t idx, void * data_pin)
{
    cstr_val_t * data = (cstr_val_t *)userdata;
    return (strcmp(data[idx].cstr, (char *)data_pin));
}

int
ublox_classid_cstr2val(char * buf, size_t size, uint8_t * p_class, uint8_t *p_id)
{
    cstr_val_t list_id_mon[] = {
        {"HW", 0x09},
        {"HW2", 0x0B},
        {"IO", 0x02}, //
        {"MSGPP", 0x06}, //
        {"RXBUF", 0x07}, //
        {"RXR", 0x21}, //
        {"TXBUF", 0x08}, //
        {"VER", 0x04},
    };
    cstr_val_t list_id_cfg[] = {
        {"ANT", 0x13},
        {"BDS", 0x4A},
        {"CFG", 0x09},
        {"DAT", 0x06},
        {"EKF", 0x12}, //
        {"ESFGWT", 0x29}, //
        {"FXN", 0x0E},
        {"INF", 0x02},
        {"ITFM", 0x39}, //
        {"MSG", 0x01},
        {"NAV5", 0x24},
        {"NAVX5", 0x23},
        {"NMEA", 0x17},
        {"NVS", 0x22}, //
        {"PM", 0x32},
        {"PM2", 0x3B}, //
        {"PRT", 0x00},
        {"RATE", 0x08},
        {"RINV", 0x34},
        {"RXM", 0x11},
        {"SBAS", 0x16},
        {"TMODE", 0x1D}, //
        {"TMODE2", 0x3D}, //
        {"TP", 0x07},
        {"TP5", 0x31},
        {"USB", 0x1B},
    };
    cstr_val_t list_id_dbg[] = {
        {"SET", 0x01},
    };

    cstr_val_t list_class[] = {
        {"CFG", 0x06},
        {"DBG", 0x09},
        {"MON", 0x0A},
    };
    record_cstr_val_t ublox_class_id[] = {
        { &list_id_cfg, NUM_ARRAY(list_id_cfg), },
        { &list_id_dbg, NUM_ARRAY(list_id_dbg), },
        { &list_id_mon, NUM_ARRAY(list_id_mon), },
    };

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
        fprintf(stderr, "warning: not found UBX class-id separator\n");
        return -1;
    }
    memset(buffer, 0, sizeof(buffer));
    strncpy(buffer, buf, size);
    p = (buffer + (p - buf));
    *p = 0;

    if (0 > pf_bsearch_r (list_class, NUM_ARRAY(list_class), pf_bsearch_cb_comp_cstrval, buffer, &idx1)) {
        // not found
        fprintf(stderr, "warning: not found UBX class '%s'\n", buffer);
        return -1;
    }
    *p_class = list_class[idx1].val;
    if (0 > pf_bsearch_r (ublox_class_id[idx1].cstrval, ublox_class_id[idx1].sz_val, pf_bsearch_cb_comp_cstrval, p+1, &idx2)) {
        // not found
        fprintf(stderr, "warning: not found UBX class '%s', id '%s'\n", buffer, p+1);
        return -1;
    }
    *p_id = ublox_class_id[idx1].cstrval[idx2].val;
    return 0;
}

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
process_command_line_buf_ubloxhex(char * buf, size_t size, uint8_t * buf_out, size_t sz_bufout)
{
    uint8_t class;
    uint8_t id;
    uint16_t len;
    char *p = buf;
    char *p_end = NULL;
    int i;
    char buffer[20];
    unsigned int val;

    // find the " - "
    p_end = strstr(buf, " - ");
    if (NULL == p_end) {
        return -1;
    }
    assert (sizeof(buffer) > (p_end - buf));
    memset(buffer, 0, sizeof(buffer));
    strncpy(buffer, buf, (p_end - buf));

    if (0 > ublox_classid_cstr2val(buffer, (p_end - buf), &class, &id)) {
        // not found
        fprintf(stderr, "not found class: '%s'\n", buffer);
        return -1;
    }

    if (sz_bufout < 8) {
        fprintf(stderr, "output buffer size too small: '%d'\n", sz_bufout);
        return -2;
    }
    buf_out[0] = 0xB5;
    buf_out[1] = 0x62;
    p = p_end + 2;
    for (i = 2; ; i ++) {
        p = strchr(p, ' ');
        if (NULL == p) {
            break;
        }
        p ++;
        if (sscanf(p, "%x", &val) <= 0) {
            break;
        }
        if (sz_bufout < i) {
            fprintf(stderr, "output buffer size too small: '%d' at pos=%d\n", sz_bufout, i);
            return -2;
        }
        // got value
        buf_out[i] = val & 0xFF;
    }
    assert (i >= 8);
    // check the class,id,length
    len = (((unsigned int)buf_out[5]) << 8) | buf_out[4];

    assert (len + 2 + 4 == i);
    assert (class == buf_out[2]);
    assert (id == buf_out[3]);

    ublox_pkt_checksum(buf_out + 2, i - 2, buf_out + i);
    i += 2;

    return i;
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
process_command_line_buf_rtklibarg(char * buf_in, size_t sz_bufin, char * buf_out, size_t sz_bufout)
{
    ssize_t ret = -1;
    uint8_t class;
    uint8_t id;
    char *p_end = NULL;
    char buf_prefix[20];

#define CSTR_CUR_COMMAND "!UBX"
    if (0 != STRCMP_STATIC (buf_in, CSTR_CUR_COMMAND)) {
        // not a rtklib ubx command
        return -1;
    }
    // find the " "
    p_end = strchr(buf_in + sizeof(CSTR_CUR_COMMAND), ' ');
    if (NULL == p_end) {
        return -1;
    }

    memset(buf_prefix, 0, sizeof(buf_prefix));
    strncpy(buf_prefix, buf_in + sizeof(CSTR_CUR_COMMAND), p_end - buf_in - sizeof(CSTR_CUR_COMMAND));
#undef CSTR_CUR_COMMAND

    if (0 > ublox_classid_cstr2val(buf_prefix, strlen(buf_prefix), &class, &id)) {
        // not found
        fprintf(stderr, "not found class: '%s'\n", buf_prefix);
        return -1;
    }
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
    case UBX_DBG_SET:
    {
        unsigned int u4_1;
        unsigned int u4_2;
        unsigned int u4_3;
        unsigned int u2_1;
        unsigned int class;
        unsigned int id;

        sscanf(p_end, "%d %d %d %d %d %d", &u4_1, &u4_2, &u4_3, &u2_1, &class, &id);
        fprintf(stderr, "ublox_pkt_create_dbg_set(0x%08X 0x%08X 0x%08X 0x%04X 0x%02X 0x%02X) from '%s'\n", u4_1, u4_2, u4_3, u2_1, class, id, p_end);
        ret = ublox_pkt_create_dbg_set (buf_out, sz_bufout, u4_1, u4_2, u4_3, u2_1, class, id);
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

        sscanf(p_end, "%d %d %d %d %d %d", &u4_1, &u4_2, &u4_3, &u4_4, &u4_5, &u4_6);
        fprintf(stderr, "ublox_pkt_create_cfg_bds(0x%08X 0x%08X 0x%08X 0x%08X 0x%08X 0x%08X) from '%s'\n", u4_1, u4_2, u4_3, u4_4, u4_5, u4_6, p_end);
        ret = ublox_pkt_create_cfg_bds (buf_out, sz_bufout, u4_1, u4_2, u4_3, u4_4, u4_5, u4_6);
    }
        break;

    case UBX_CFG_MSG:
    {
        int i;
        char *p;
        uint8_t buf1[8];
        memset(buf1, 0, sizeof(buf1));
        p = p_end-1;
        for (i = 0; i < 8; i ++) {
            p = strchr(p, ' ');
            if (NULL != p) {
                while (*p == ' ') p ++;
                buf1[i] = atoi(p);
            } else {
                break;
            }
        }
        if (i < 8) {
            // get conf
            i = 0;
        }
        ret = ublox_pkt_create_set_cfgmsg (buf_out, sz_bufout, buf1[0], buf1[1], buf1 + 2, 6);
    }
        break;
    case UBX_CFG_PRT:
    {
        unsigned int portID = 0xFF;

        int i;
        char *p;

        p = p_end-1;
        for (i = 0; i < 6; i ++) {
            p = strchr(p, ' ');
            if (NULL != p) {
                while (*p == ' ') p ++;
            } else {
                break;
            }
            if (portID == 0xFF) {
                portID = atoi(p);
            }
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
            sscanf(p_end, "%d %d %d %d %d %d", &portID, &txReady, &mode, &baudRate, &inProtoMask, &outProtoMask);
            ret = ublox_pkt_create_set_cfgprt (buf_out, sz_bufout, portID, txReady, mode, baudRate, inProtoMask, outProtoMask);
        }
    }
        break;
    case UBX_CFG_RATE:
    {
        int i;
        char *p;

        p = p_end-1;
        for (i = 0; i < 3; i ++) {
            p = strchr(p, ' ');
            if (NULL != p) {
                while (*p == ' ') p ++;
            } else {
                break;
            }
        }
        if (i < 3) {
            // get conf
            ret = ublox_pkt_create_get_cfgrate(buf_out, sz_bufout);
        } else {
            unsigned int measRate;
            unsigned int navRate;
            unsigned int timeRef;
            sscanf(p_end, "%d %d %d", &measRate, &navRate, &timeRef);
            ret = ublox_pkt_create_set_cfgrate (buf_out, sz_bufout, measRate, navRate, timeRef);
        }
    }
        break;
    default:
        ret = -1;
        break;
    }

    return ret;
}

/**
 * \brief parse the lines in the buffer and send out packets base on the command
 * \param pos: the position in the file
 * \param buf: the buffer
 * \param size: the size of buffer
 * \param userdata: the pointer passed by the user
 *
 * \return 0 on successs, <0 on error
 *
 * TODO: add command 'sleep' to support delay between commands.
 */
int
process_command_libuv(off_t pos, char * buf, size_t size, void *userdata)
{
    uv_stream_t* stream = (uv_stream_t *)userdata;

    ssize_t ret = -1;
    uint8_t buffer1[200];

    fprintf(stderr, "ubxcli process line: '%s'\n", buf);

    ret = process_command_line_buf_rtklibarg(buf, size, buffer1, sizeof(buffer1));
    if (ret < 0) {
        ret = process_command_line_buf_ubloxhex(buf, size, buffer1, sizeof(buffer1));
    }
    if (ret < 0) {
        fprintf(stderr, "tcp cli ignore line at pos(%ld): %s\n", pos, buf);
        return 0;
    }
#if DEBUG
    fprintf(stderr, "---------------------------------------------\n");
    fprintf(stderr, "tcp cli created packet size=%" PRIiSZ ":\n", ret);
    hex_dump_to_fd(STDERR_FILENO, (opaque_t *)(buffer1), ret);
    {
        size_t sz_processed;
        size_t sz_needed_in;
        ublox_cli_verify_tcp(buffer1, ret, &sz_processed, &sz_needed_in);
    }
    fprintf(stderr, "---------------------------------------------\n");
    assert (ret <= sizeof(buffer1));
#endif

    send_buffer(stream, buffer1, ret);
    g_ubxcli.num_requests ++;

    return 0;
}

void
on_tcp_cli_connect(uv_connect_t* connection, int status)
{
    uv_stream_t* stream = connection->handle;

    fprintf(stderr, "tcp cli connected.\n");

    read_file_lines (g_ubxcli.fn_execute, (void *)stream, process_command_libuv);
    uv_read_start(stream, alloc_buffer, on_tcp_cli_read);
}

/*****************************************************************************/

static char flg_has_error = 0;

static void
idle_cb (uv_idle_t *handle)
{
    time_t curtime;
    time(&curtime);
    if (g_ubxcli.starttime + g_ubxcli.timeout <= curtime) {
        flg_has_error = 1;
        uv_idle_stop(handle);
        uv_stop(uv_default_loop());
        fprintf(stderr, "timeout: %d\n", (int)g_ubxcli.timeout);
    }
}

static void
on_uv_close(uv_handle_t* handle)
{
    if (handle != NULL) {
        //delete handle;
    }
}

static void
on_uv_walk(uv_handle_t* handle, void* arg)
{
    uv_close(handle, on_uv_close);
}

static void
on_sigint_received(uv_signal_t *handle, int signum)
{
    int result = uv_loop_close(handle->loop);
    if (result == UV_EBUSY) {
        uv_walk(handle->loop, on_uv_walk, NULL);
    }
}

/*****************************************************************************/
int
main_cli(const char * host, int port_tcp, time_t timeout, const char * fn_execute)
{
    int ret = 0;
    struct sockaddr_in broadcast_addr;
    struct sockaddr_in addr_udp;
    uint32_t connect_code = 0;
    uv_idle_t idler;
    uv_signal_t sigint;

    // setup service related info
    memset (&g_ubxcli, 0, sizeof (g_ubxcli));
    g_ubxcli.sz_data = 0;
    g_ubxcli.num_requests = 0;
    g_ubxcli.num_responds = 0;
    g_ubxcli.fn_execute = fn_execute;
    uv_ip4_addr(host, port_tcp, &(g_ubxcli.addr_tcp));

    loop = uv_default_loop();
    assert (NULL != loop);

    uv_signal_init(loop, &sigint);
    uv_signal_start(&sigint, on_sigint_received, SIGINT);
    if (timeout > 0) {
        uv_idle_init(loop, &idler);
        uv_idle_start(&idler, idle_cb);
    }
    time (&(g_ubxcli.starttime));
    g_ubxcli.timeout = timeout;

    uv_tcp_init(loop, &(g_ubxcli.uvtcp));
    uv_tcp_keepalive(&(g_ubxcli.uvtcp), 1, 60);

    uv_tcp_connect(&(g_ubxcli.connect), &(g_ubxcli.uvtcp), (const struct sockaddr*)&(g_ubxcli.addr_tcp), on_tcp_cli_connect);

    ret = uv_run(loop, UV_RUN_DEFAULT);
    // uv_signal_stop(&sigint);
    if (ret != 0) {
        return ret;
    }
    if (flg_has_error) {
        return 1;
    }
    return 0;
}

/*****************************************************************************/
/**
 * \brief parse the lines in the buffer and send out packets base on the command
 * \param pos: the position in the file
 * \param buf: the buffer
 * \param size: the size of buffer
 * \param userdata: the pointer passed by the user
 *
 * \return 0 on successs, <0 on error
 *
 * TODO: add command 'sleep' to support delay between commands.
 */
int
process_command_stdout(off_t pos, char * buf, size_t size, void *userdata)
{
    FILE *fp = (FILE *)userdata;
    ssize_t ret = -1;
    uint8_t buffer1[200];

    fprintf(stderr, "ubxcli process line: '%s'\n", buf);

    ret = process_command_line_buf_rtklibarg(buf, size, buffer1, sizeof(buffer1));
    if (ret < 0) {
        ret = process_command_line_buf_ubloxhex(buf, size, buffer1, sizeof(buffer1));
    }
    if (ret < 0) {
        fprintf(stderr, "tcp cli ignore line at pos(%ld): %s\n", pos, buf);
        return 0;
    }
    fprintf(stderr, "---------------------------------------------\n");
    fprintf(stderr, "tcp cli created packet size=%" PRIiSZ ":\n", ret);
    hex_dump_to_fd(STDERR_FILENO, (opaque_t *)(buffer1), ret);
    {
        size_t sz_processed;
        size_t sz_needed_in;
        ublox_cli_verify_tcp(buffer1, ret, &sz_processed, &sz_needed_in);
    }
    fprintf(stderr, "---------------------------------------------\n");
    return 0;
}

/*****************************************************************************/
void
version (void)
{
    fprintf (stdout, "UBlox module configure utils\n");
    fprintf (stdout, "Version %d.%d.%d\n", VER_MAJOR, VER_MINOR, VER_MOD);
    fprintf (stdout, "Copyright (c) 2018 Y. Fu. All rights reserved.\n\n");
}

void
help (char *progname)
{
    fprintf (stdout, "Usage: \n"
        "\t%s [-hv] [-r <host>[:port]] [commands...]\n"
        , basename(progname));
    fprintf (stdout, "\nOptions:\n");
    fprintf (stdout, "\t-r\tRemote host and port\n");
    fprintf (stdout, "\t-e <cmd file>\tExecute the command lines in the file\n");
    fprintf (stdout, "\t-t <timeout>\tThe seconds before quit, 0 - wait forever, default 30\n");

    fprintf (stdout, "\t-h\tPrint this message.\n");
    fprintf (stdout, "\t-v\tVerbose information.\n");
    fprintf (stdout, "\nExamples: \n"
        "\t1. connect and display basic information\n"
        "\t\t%s -r localhost:23\n\n"
        "\t2. connect and execute command\n"
        "\t\t%s -r localhost:23 reset\n\n"
        , basename(progname), basename(progname));
}

void
usage (char *progname)
{
    version ();
    help (progname);
}

int
main (int argc, char **argv)
{
    const char host[200] = "";
    int port = UBLOX_PORT_DEFAULT;
    const char * fn_execute = NULL;
    time_t timeout = 30;

    int c;
    struct option longopts[]  = {
        { "remote",       1, 0, 'r' },
        { "execute",      1, 0, 'e' },
        { "timeout",      1, 0, 't' },

        { "help",         0, 0, 'h' },
        { "verbose",      0, 0, 'v' },
        { 0,              0, 0,  0  },
    };

    while ((c = getopt_long( argc, argv, "r:e:t:vh", longopts, NULL )) != EOF) {
        switch (c) {
        case 'r':
        {
            char *p;
            strncpy(host, optarg, sizeof(host)-1);
            p = strchr(host, ':');
            if (0 != p) {
                *p = 0;
                p ++;
                port = atoi(p);
            }
        }
            break;
        case 'e':
            if (strlen (optarg) > 0) {
                fn_execute = optarg;
            }
            break;

        case 't':
            timeout = atoi(optarg);
            break;

        case 'h':
            usage (argv[0]);
            exit (0);
            break;
        case 'v':
            break;

        default:
            fprintf (stderr, "Unknown parameter: '%c'.\n", c);
            fprintf (stderr, "Use '%s -h' for more information.\n", basename(argv[0]));
            exit (-1);
            break;
        }
    }

    if (strlen(host) < 1) {
        if (fn_execute) {
            // parse the execute file
            read_file_lines (fn_execute, (void *)stdout, process_command_stdout);
        }
        return 0;
    }
    return main_cli(host, port, timeout, fn_execute);
}

