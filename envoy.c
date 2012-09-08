/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright (C) Simon Gomizelj, 2012
 */

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <getopt.h>
#include <err.h>
#include <errno.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/un.h>

enum action {
    ACTION_PRINT,
    ACTION_ADD,
    ACTION_FORCE_ADD,
    ACTION_KILL,
    ACTION_INVALID
};

static void ssh_key_add(int argc, char *argv[])
{
    char *args[argc + 3];
    int i;

    for (i = 0; i < argc; ++i)
        args[i + 2] = argv[i];

    args[0] = "ssh-add";
    args[1] = "--";
    args[argc + 2] = NULL;

    if (execvp(args[0], args) < 0)
        err(EXIT_FAILURE, "failed to start ssh-add");
}

static int get_agent(struct agent_data_t *data)
{
    union {
        struct sockaddr sa;
        struct sockaddr_un un;
    } sa;

    int fd = socket(PF_UNIX, SOCK_STREAM, 0);
    if (fd < 0)
        err(EXIT_FAILURE, "couldn't create socket");

    memset(&sa, 0, sizeof(sa));
    sa.un.sun_family = AF_UNIX;
    strncpy(sa.un.sun_path, SOCK_PATH, sizeof(sa.un.sun_path));

    if (connect(fd, &sa.sa, sizeof(sa)) < 0)
        err(EXIT_FAILURE, "failed to connect");

    int rc = read(fd, data, sizeof(*data));

    close(fd);
    return rc;
}

static void __attribute__((__noreturn__)) usage(FILE *out)
{
	fprintf(out, "usage: %s [options] [files ...]\n", program_invocation_short_name);
	fputs("Options:\n"
	      " -h, --help       display this help and exit\n"
	      " -v, --version    display version\n"
	      " -a, --add        always invode ssh-add, not just on ssh-agent start\n"
	      " -k, --kill       kill the running ssh-agent\n"
	      " -p, --print      print out environmental arguments\n", out);

	exit(out == stderr ? EXIT_FAILURE : EXIT_SUCCESS);
}

int main(int argc, char *argv[])
{
    struct agent_data_t data;
    enum action verb = ACTION_ADD;

    static const struct option opts[] = {
        { "help",    no_argument, 0, 'h' },
        { "version", no_argument, 0, 'v' },
        { "add",     no_argument, 0, 'a' },
        { "kill",    no_argument, 0, 'k' },
        { "print",   no_argument, 0, 'p' },
        { 0, 0, 0, 0 }
    };

    while (true) {
        int opt = getopt_long(argc, argv, "hvakp", opts, NULL);
        if (opt == -1)
            break;

        switch (opt) {
        case 'h':
            usage(stdout);
            break;
        case 'v':
            printf("%s %s\n", program_invocation_short_name, ENVOY_VERSION);
            return 0;
        case 'a':
            verb = ACTION_FORCE_ADD;
            break;
        case 'k':
            verb = ACTION_KILL;
            break;
        case 'p':
            verb = ACTION_PRINT;
            break;
        default:
            usage(stderr);
        }
    }

    argv += optind;
    argc -= optind;

    if (get_agent(&data) < 0)
        err(EXIT_FAILURE, "failed to read data");

    setenv("SSH_AUTH_SOCK", data.sock, true);

    switch (verb) {
    case ACTION_PRINT:
        printf("export SSH_AUTH_SOCK=%s\n",  data.sock);
        printf("export SSH_AGENT_PID=%ld\n", (long)data.pid);
        break;
    case ACTION_ADD:
        if (!data.first_run)
            return 0;
    case ACTION_FORCE_ADD:
        ssh_key_add(argc, argv);
        break;
    case ACTION_KILL:
        kill(data.pid, SIGTERM);
        break;
    default:
        break;
    }

    return 0;
}

// vim: et:sts=4:sw=4:cino=(0
