#ifndef _MUTEBOT_H
#define _MUTEBOT_H

#include <stdbool.h>
#include "config.h"

struct cmdline {
    char *td_path;
    bool test_dc;
    int api_id;
    char *api_hash;
    char *bot_token;
    bool logout;
};

extern struct cmdline cmd;

int cmdline_init(int argc, char **argv);

#endif /* _MUTEBOT_H */
