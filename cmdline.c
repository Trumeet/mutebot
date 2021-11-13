#include "mutebot.h"
#include <unistd.h>
#include <stdio.h>
#include <sysexits.h>
#include <inttypes.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

struct cmdline cmd = {
        "./td_data/",
        false,
        -1,
        NULL,
        NULL,
        false
};

static inline int parse_api_id(char *str) {
    char *endptr;
    intmax_t num = strtoimax(str, &endptr, 10);
    if (strcmp(endptr, "") != 0 || (num == INTMAX_MAX && errno == ERANGE) ||
        num > INT_MAX || num < INT_MIN) {
        fprintf(stderr, "Invalid API Hash: %s\n", optarg);
        return EX_USAGE;
    }
    cmd.api_id = (int) num;
    return 0;
}

int cmdline_init(int argc, char **argv) {
    int opt;
    while ((opt = getopt(argc, argv, "d:ti:H:T:l")) != -1) {
        switch (opt) {
            case 'd':
                cmd.td_path = optarg;
                break;
            case 't':
                cmd.test_dc = true;
                break;
            case 'i': {
                int r = parse_api_id(optarg);
                if (r) return r;
                break;
            }
            case 'H':
                cmd.api_hash = optarg;
                break;
            case 'T':
                cmd.bot_token = optarg;
                break;
            case 'l':
                cmd.logout = true;
                break;
            default:
                fprintf(stderr, "Consult mutebot(1) for more details.\n");
                return EX_USAGE;
        }
    }
    if (cmd.api_id == -1 && getenv("TD_API_ID") != NULL) {
        int r = parse_api_id(getenv("TD_API_ID"));
        if (r) return r;
    }
    if (cmd.api_hash == NULL) cmd.api_hash = getenv("TD_API_HASH");
    if (cmd.bot_token == NULL) cmd.bot_token = getenv("TD_BOT_TOKEN");

    bool chkfail = false;
    if (cmd.api_id == -1) {
        fprintf(stderr, "You need to specify -i <API ID> or use TD_API_ID Environment Variable.\n");
	chkfail = true;
    }
    if (cmd.api_hash == NULL) {
        fprintf(stderr, "You need to specify -H <API Hash> or use TD_API_HASH Environment Variable.\n");
	chkfail = true;
    }
    if (cmd.bot_token == NULL) {
        fprintf(stderr, "You need to specify -T <BOT Token> or use TD_BOT_TOKEN Environment Variable.\n");
	chkfail = true;
    }
    return chkfail ? EX_USAGE : EX_OK;
}
