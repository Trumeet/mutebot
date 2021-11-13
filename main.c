#include "mutebot.h"
#include "tdutils.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

int post_auth() {

}

int main(int argc, char **argv) {
    int r = 0;
    if ((r = cmdline_init(argc, argv))) goto cleanup;
    if ((r = td_init())) goto cleanup;
    td_loop();
    goto cleanup;
    cleanup:
    td_free();
    return r;
}
