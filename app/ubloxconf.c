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
                g_ubxcli.num_responds ++;
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
process_command(off_t pos, char * buf, size_t size, void *userdata)
{
    uv_stream_t* stream = (uv_stream_t *)userdata;
    ssize_t ret = -1;
    uint8_t buffer1[100];
    uint8_t buffer2[100];
    uint32_t address = 0xFF;
    ssize_t count = sizeof(buffer2);
    char * endptr = NULL;

    fprintf(stderr, "ubxcli process line: '%s'\n", buf);

    if (0 == STRCMP_STATIC (buf, "!UBX MON-VER")) {
        ret = ublox_pkt_create_get_version (buffer1, sizeof(buffer1));

#define CSTR_CUR_COMMAND "!UBX MON-HW"
    } else if (0 == STRCMP_STATIC (buf, CSTR_CUR_COMMAND)) {
        ret = ublox_pkt_create_get_hw (buffer1, sizeof(buffer1));
#undef CSTR_CUR_COMMAND

#define CSTR_CUR_COMMAND "!UBX MON-HW2"
    } else if (0 == STRCMP_STATIC (buf, CSTR_CUR_COMMAND)) {
        ret = ublox_pkt_create_get_hw2 (buffer1, sizeof(buffer1));
#undef CSTR_CUR_COMMAND

#define CSTR_CUR_COMMAND "!UBX UNK-MG1"
    } else if (0 == STRCMP_STATIC (buf, CSTR_CUR_COMMAND)) {
        unsigned int u4_1;
        unsigned int u4_2;
        unsigned int u4_3;
        unsigned int u2_1;
        unsigned int class;
        unsigned int id;

        sscanf(buf + sizeof(CSTR_CUR_COMMAND), "%X %X %X %X %X %X", &u4_1, &u4_2, &u4_3, &u2_1, &class, &id);
        fprintf(stderr, "ublox_pkt_create_unknown_msg1(0x%08X 0x%08X 0x%08X 0x%04X 0x%02X 0x%02X) from '%s'\n", u4_1, u4_2, u4_3, u2_1, class, id, buf + sizeof(CSTR_CUR_COMMAND));
        ret = ublox_pkt_create_unknown_msg1 (buffer1, sizeof(buffer1), u4_1, u4_2, u4_3, u2_1, class, id);
#undef CSTR_CUR_COMMAND

#define CSTR_CUR_COMMAND "!UBX CFG-MSG"
    } else if (0 == STRCMP_STATIC (buf, CSTR_CUR_COMMAND)) {
        int i;
        char *p;
        uint8_t buf1[8];
        memset(buf1, 0, sizeof(buf1));
        p = buf + sizeof(CSTR_CUR_COMMAND)-1;
        for (i = 0; i < 8; i ++) {
            p = strchr(p, ' ');
            if (NULL != p) {
                while (*p == ' ') p ++;
                buf1[i] = atoi(p);
            } else {
                break;
            }
        }
        ret = ublox_pkt_create_set_cfgmsg (buffer1, sizeof(buffer1), buf1[0], buf1[1], buf1 + 2, 6);
#undef CSTR_CUR_COMMAND

    }
    if (ret < 0) {
        fprintf(stderr, "tcp cli ignore line at pos(%ld): %s\n", pos, buf);
        return 0;
    }
    fprintf(stderr, "tcp cli created packet size=%" PRIiSZ ":\n", ret);
    hex_dump_to_fd(STDERR_FILENO, (opaque_t *)(buffer1), ret);
    assert (ret <= sizeof(buffer1));
    send_buffer(stream, buffer1, ret);
    g_ubxcli.num_requests ++;
    return 0;
}

void
on_tcp_cli_connect(uv_connect_t* connection, int status)
{
    uv_stream_t* stream = connection->handle;

    fprintf(stderr, "tcp cli connected.\n");

    read_file_lines (g_ubxcli.fn_execute, (void *)stream, process_command);
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
    const char host[200] = "127.0.0.1";
    int port = UBLOX_PORT_DEFAULT;
    const char * fn_execute = NULL;
    time_t timeout = 0;

    int c;
    struct option longopts[]  = {
        { "remote",       1, 0, 'r' },
        { "execute",      1, 0, 'e' },

        { "help",         0, 0, 'h' },
        { "verbose",      0, 0, 'v' },
        { 0,              0, 0,  0  },
    };

    while ((c = getopt_long( argc, argv, "r:e:vh", longopts, NULL )) != EOF) {
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

    return main_cli(host, port, timeout, fn_execute);
}

