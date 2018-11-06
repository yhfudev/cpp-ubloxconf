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

#include <stdio.h>
#include <stdlib.h> // exit()
#include <getopt.h>
#include <libgen.h> // basename()

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
    int c;
    struct option longopts[]  = {
        { "remote",       1, 0, 'r' },

        { "help",         0, 0, 'h' },
        { "verbose",      0, 0, 'v' },
        { 0,              0, 0,  0  },
    };

    while ((c = getopt_long( argc, argv, "r:vh", longopts, NULL )) != EOF) {
        switch (c) {
        case 'r':
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
    return 0;
}

