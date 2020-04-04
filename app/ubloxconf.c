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

#include "ubloxutils.h"
#include "ubloxconn.h"
#include "ubloxcstr.h"

#undef DEBUG
#define DEBUG 1

#if DEBUG
#include "hexdump.h"
#endif
#ifndef hex_dump_to_fd
#define hex_dump_to_fd(a,b,c)
#endif
#ifndef hex_dump_to_fp
#define hex_dump_to_fp(a,b,c)
#endif

#define nullptr NULL
#define TRACE(fmt, ...) {time_t now = time(nullptr); struct tm timeinfo; char buf[30]; gmtime_r(&now, &timeinfo); strcpy(buf, asctime(&timeinfo)); buf[strlen(buf)-1] = 0; fprintf (stderr, "%s [%s()] " fmt " {ln:%d, fn:" __FILE__ "}\n", buf, __func__, ##__VA_ARGS__, __LINE__); }


#define TD TRACE
#define TI TRACE

//#define TD(...)
//#define TI(...)
//#define TD printf
//#define TI printf
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

    TD("tcp cli ubxcli_process_data() BEGIN\n");
    while(1) { // while (ped->sz_data > 0) {
        buffer_in = ped->buffer;
        sz_in = ped->sz_data;
        //flg_again = 0;
        sz_processed = 0;
        sz_needed_in = 0;

        TD("tcp cli ubxcli_process_data() ped->sz_data=%" PRIuSZ "\n", ped->sz_data);
        assert (NULL != buffer_in);
        assert (sz_in >= 0);
        TD("tcp cli ubxcli_process_data() call ublox_pkt_nexthdr_ubx\n");

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
            TI( "need more data: %" PRIuSZ "\n", sz_needed_in);
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
                TI( "need more data: %" PRIuSZ "\n", sz_needed_in);
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
    TI( "tcp cli closed.\n");
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

        TD("tcp cli read block, size=%" PRIiSZ ":\n", nread);
        hex_dump_to_fd(STDERR_FILENO, (opaque_t *)(buf->base), nread);

        TD("tcp cli process data 1\n");
        ubxcli_process_data (&g_ubxcli, stream);
        sz_processed = 0;
        while (sz_processed < nread) {
            sz_copy = nread - sz_processed;
            assert (sizeof(g_ubxcli.buffer) >= g_ubxcli.sz_data);
            if (sz_copy + g_ubxcli.sz_data > sizeof(g_ubxcli.buffer)) {
                sz_copy = sizeof(g_ubxcli.buffer) - g_ubxcli.sz_data;
            }
            TD("tcp cli push received data size=%" PRIuSZ ", processed=%" PRIuSZ "\n", sz_copy, sz_processed);
            if (sz_copy > 0) {
                memmove (g_ubxcli.buffer + g_ubxcli.sz_data, buf->base + sz_processed, sz_copy);
                sz_processed += sz_copy;
                g_ubxcli.sz_data += sz_copy;
            } else {
                // error? full?
                TI( "tcp cli no more received data to be push\n");
                break;
            }
            TD("tcp cli process data 2\n");
            ubxcli_process_data (&g_ubxcli, stream);
        }
        if (sz_processed < nread) {
            // we're stalled here, because the content can't be processed by the function ubxcli_process_data()
            // error
            TD("tcp cli data stalled\n");
            //uv_close((uv_handle_t *)stream, on_tcp_cli_close);
        }
    }
    if (nread == 0) {
        TI("tcp cli read zero!\n");
    }
    if (nread < 0) {
        //we got an EOF
        TI("tcp cli read EOF!\n");
        uv_close((uv_handle_t*)stream, on_tcp_cli_close);
    }

    free(buf->base);
    if (g_ubxcli.num_responds >= g_ubxcli.num_requests) {
        TI("tcp cli received responses(%" PRIuSZ ") exceed requests(%" PRIuSZ ")!\n", g_ubxcli.num_responds, g_ubxcli.num_requests);
        uv_close((uv_handle_t*)stream, on_tcp_cli_close);
        raise(SIGINT); // send signal and handle by uv_signal_cb
    }
}

void
on_tcp_cli_write_end(uv_write_t* req, int status)
{
    if (status) {
        TI( "tcp cli write error %s.\n", uv_strerror(status));
        uv_close(req->handle, on_tcp_cli_close);
        return;
    } else {
        TI( "tcp cli write successfull.\n");
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
        TE( "tcp cli error in write() %s\n", uv_strerror(r));
    }
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

    TD("ubxcli process line: '%s'\n", buf);

    ret = ublox_confline2bin_rtklibarg(buf, size, buffer1, sizeof(buffer1));
    if (ret < 0) {
        ret = ublox_confline2bin_hex(buf, size, buffer1, sizeof(buffer1));
    }
    if (ret < 0) {
        TI("tcp cli ignore line at pos(%ld): %s\n", pos, buf);
        return 0;
    }
#if DEBUG
    TD("---------------------------------------------\n");
    TD("tcp cli created packet size=%" PRIiSZ ":\n", ret);
    hex_dump_to_fd(STDERR_FILENO, (opaque_t *)(buffer1), ret);
    {
        size_t sz_processed;
        size_t sz_needed_in;
        ublox_cli_verify_tcp(buffer1, ret, &sz_processed, &sz_needed_in);
    }
    TD("---------------------------------------------\n");
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

    TD("tcp cli connected.\n");

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
        TW("timeout: %d\n", (int)g_ubxcli.timeout);
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

    TD("ubxcli process line: '%s'\n", buf);

    ret = ublox_confline2bin_rtklibarg(buf, size, buffer1, sizeof(buffer1));
    if (ret < 0) {
        ret = ublox_confline2bin_hex(buf, size, buffer1, sizeof(buffer1));
    }
    if (ret < 0) {
        TI("tcp cli ignore line at pos(%ld): %s\n", pos, buf);
        return 0;
    }
    TD("---------------------------------------------\n");
    TD("tcp cli created packet size=%" PRIiSZ ":\n", ret);
    hex_dump_to_fd(STDERR_FILENO, (opaque_t *)(buffer1), ret);
    {
        size_t sz_processed;
        size_t sz_needed_in;
        ublox_cli_verify_tcp(buffer1, ret, &sz_processed, &sz_needed_in);
    }
    TD("---------------------------------------------\n");
    return 0;
}

void
decode_bin(FILE *fp)
{
    uint8_t buffer1[200];
    size_t sz_processed;
    size_t sz_needed_in;
    size_t sz_in;
    ssize_t ret;
    size_t sz_cur = 0;

    sz_in = 0;
    for(;;) {
        ret = fread(buffer1 + sz_in, 1, sizeof(buffer1) - sz_in, fp);
        if (ret > 0) {
            sz_in += ret;
            fprintf(stderr, "[ubloxconf] processed data size = %lu\n", sz_cur);
        } else {
            break;
        }
        while (sz_in > 0) {
            sz_processed = 0;
            sz_needed_in = 0;
            ublox_cli_verify_tcp(buffer1, ret, &sz_processed, &sz_needed_in);
            if (sz_processed > 0) {
                if (sz_in > sz_processed) {
                    sz_in -= sz_processed;
                    sz_cur += sz_processed;
                    memmove(buffer1, buffer1 + sz_processed, sz_in);
                } else {
                    sz_in = 0;
                }
            }
            if (sz_needed_in > 0) {
                break;
            }
        }
    }
    fprintf(stderr, "[ubloxconf] processed data size = %lu\n", sz_cur);
}

/*****************************************************************************/

#if defined(CIUT_ENABLED) && (CIUT_ENABLED == 1)
#else
void
version (void)
{
    fprintf (stderr, "UBlox module configure utils\n");
    fprintf (stderr, "Version %d.%d.%d\n", VER_MAJOR, VER_MINOR, VER_MOD);
    fprintf (stderr, "Copyright (c) 2018 Y. Fu. All rights reserved.\n\n");
}

void
help (char *progname)
{
    fprintf (stderr, "Usage: \n"
        "\t%s [-hv] [-r <host>[:port]] [commands...]\n"
        , basename(progname));
    fprintf (stderr, "\nOptions:\n");
    fprintf (stderr, "\t-r\tRemote host and port\n");
    fprintf (stderr, "\t-e <cmd file>\tExecute/encode the text command lines in the file\n");
    fprintf (stderr, "\t-d <cmd file>\tDecode the binary packet from file or stdin\n");
    fprintf (stderr, "\t-t <timeout>\tThe seconds before quit, 0 - wait forever, default 30\n");

    fprintf (stderr, "\t-h\tPrint this message.\n");
    fprintf (stderr, "\t-v\tVerbose information.\n");
    fprintf (stderr, "\nExamples: \n"
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
    FILE * fp_bin = stdin;
    time_t timeout = 30;

    int c;
    struct option longopts[]  = {
        { "remote",       1, 0, 'r' },
        { "execute",      1, 0, 'e' },
        { "decode",       1, 0, 'd' },
        { "timeout",      1, 0, 't' },

        { "help",         0, 0, 'h' },
        { "verbose",      0, 0, 'v' },
        { 0,              0, 0,  0  },
    };

    while ((c = getopt_long( argc, argv, "r:e:d:t:vh", longopts, NULL )) != EOF) {
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

        case 'd':
            if (strlen (optarg) > 0) {
                if (stdin != fp_bin) {
                    fclose(fp_bin);
                }
                if (0 == strcmp("-", optarg)) {
                    fp_bin = stdin;
                } else {
                    fp_bin = fopen(optarg, "rb");
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
        } else if (fp_bin) {
            decode_bin(fp_bin);
        }
        if (stdin != fp_bin) {
            fclose(fp_bin);
        }
        return 0;
    }
    return main_cli(host, port, timeout, fn_execute);
}
#endif /* CIUT_ENABLED */
