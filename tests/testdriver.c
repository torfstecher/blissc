/*
 *++
 *	File:			testdriver.c
 *
 *	Abstract:		Test main program for the blissc driver
 *
 *  Module description:
 *                  tbd
 *
 *	Author:		M. Madison
 *				Copyright © 2013, Matthew Madison
 *				All rights reserved.
 *	Modification history:
 *		25-Jan-2013	V1.0	Madison		Initial coding.
 *--
 */
#include <stdio.h>
#include <setjmp.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include "blissc/driver.h"

static struct option options[] = {
    { "output",         required_argument,  0,  'o' },
    { "assembly",       no_argument,        0,  's' },
    { "optimization",   required_argument,  0,  'O' },
    { "listing",        optional_argument,  0,  'l' },
    { "show",           required_argument,  0,  0   },
    { "help",           no_argument,        0,  'h' },
    { 0,                0,                  0,  0   }
};
static char *optarghelp[] = {
    "--output=filename       ",
    "--assembly              ",
    "--optimization={0,1,2,3}",
    "--listing[=filename]    ",
    "--show[=option,...]     ",
    "--help                  "
};
static char *opthelp[] = {
    "output file name (defaults to source filename with '.o' or '.s' suffix)",
    "generate assembly language output instead of object",
    "set optimization level (valid values: 0,1,2,3, default:1)",
    "generate listing (default name is output file with '.lis' suffix)",
    "controls listing output (see below)",
    "displays usage information and exits"
};
static char *listopts [] = {
    "source","require","expand","trace",
    "library","object","assembly","binary",
    "commentary"
};
static char *listopthelp[] = {
    "source listing (default: on)",
    "include REQUIRE files in listing (default: off)",
    "show macro expansions (default: off)",
    "trace macro expansions (default: off)",
    "(not supported yet)",
    "(not supported yet)",
    "include assembly in listing (not supported yet)",
    "(not supported yet)",
    "(not supported yet)"
};

static char *srcfile = 0, *outfile = 0, *listfile = 0;
static unsigned int listflags = 0;
static bliss_output_t outtype = BLISS_K_OUTPUT_OBJECT;
static int optlevel = -1;

static void
print_usage (void)
{
    int i;
    printf("Usage:\n");
    printf("\tblissc [options] filename\n\n");
    printf("Options:\n");
    for (i = 0; i < sizeof(options)/sizeof(options[0]) && options[i].name != 0; i++) {
        printf(" %s\t%c%c\t%s\n",
               optarghelp[i],
               (options[i].val == 0 ? ' ' : '-'),
               (options[i].val == 0 ? ' ' : options[i].val),
               opthelp[i]);
    }
    printf("--show options:\n");
    for (i = 0; i < sizeof(listopts)/sizeof(listopts[0]); i++) {
        printf("  %-16.16s %s\n", listopts[i], listopthelp[i]);
    }

} /* print_usage */

static void
parse_args (int argc, char * const argv[])
{
    int c, which, err;
    char *showopts;

    err = 0;
    while (!err) {
        c = getopt_long_only(argc, argv, "hslo:O:", options, &which);
        if (c == -1) {
            break;
        }
        switch (c) {
            case 0: // long-only, right now this is just --show
                showopts = optarg;
                err = 0;
                while (*showopts != '\0' && !err) {
                    char *value;
                    int i = getsubopt(&showopts, listopts, &value);
                    if (i < 0) {
                        fprintf(stderr, "Unrecognized --show option: %s\n", value);
                        err = 1;
                        break;
                    }
                    listflags |= (1<<i);
                }
                break;
            case 'o':
                outfile = optarg;
                break;
            case 'l':
                listfile = optarg;
                if (listflags == 0) listflags = BLISS_M_LIST_SRC;
                break;
            case 's':
                outtype = BLISS_K_OUTPUT_ASSEMBLY;
                break;
            case 'h':
                print_usage();
                err = 999;
                break;
            case 'O':
                if (*optarg >= '0' && *optarg <= '3' && *(optarg+1) == '\0') {
                    optlevel = *optarg - '0';
                } else {
                    fprintf(stderr, "Unrecognized optimization level: %s\n", optarg);
                    err = 1;
                }
                break;
            case '?':
                fprintf(stderr, "Unrecognized option: %s\n", argv[optind]);
                err = 1;
                break;
            case ':':
                fprintf(stderr, "Required argument missing\n");
                err = 1;
                break;
            default:
                fprintf(stderr, "Unknown option processing error\n");
                err = 1;
                break;
        }

    } /* while not err */

    if (!err) {
        if (optind < argc) {
            srcfile = (char *) argv[optind];
        } else {
            fprintf(stderr, "Missing required argument (source file name)\n");
            err = 1;
        }
    }

    if (err) {
        exit((err == 999 ? 0 : err));
    }

} /* parse_args */

int
main (int argc, char * const argv[])
{
    jmp_buf retenv;
    blissc_driverctx_t cctx = 0;
    int status = 0;

    parse_args(argc, argv);

    if (setjmp(retenv)) {
        status = 1;
        goto finish;
    }
    cctx = blissc_init(retenv);
    if (!blissc_target_set(cctx, 0)) {
        fprintf(stderr, "error setting target\n");
        status = 999;
        goto finish;
    }
    if (listflags != 0 || listfile != 0) {
        if (listflags == 0) listflags = BLISS_M_LIST_SRC;
        if (!blissc_listopt_set(cctx, listflags, listfile, -1)) {
            fprintf(stderr, "Error setting listing options\n");
            status = 1;
            goto finish;
        }
    }
    if (!blissc_output_set(cctx, outtype, outfile, -1)) {
        fprintf(stderr, "Error setting output type\n");
        status = 1;
        goto finish;
    }
    if (optlevel >= 0) {
        if (!blissc_optlevel_set(cctx, optlevel)) {
            fprintf(stderr, "Error setting optimization level");
            status = 1;
            goto finish;
        }
    }
    if (!blissc_compile(cctx, srcfile, -1)) {
        fprintf(stderr, "bliss_compile reported error\n");
        status = 998;
        goto finish;
    }
finish:
    blissc_finish(cctx);
    return status;
}
