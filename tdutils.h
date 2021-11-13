#ifndef _MUTEBOT_TDUTILS_H
#define _MUTEBOT_TDUTILS_H

#include <td/telegram/td_c_client.h>

extern long long my_id;
extern bool closing;

int td_init();

void td_free();

void td_loop();

void tg_close();

int td_send(void *func, void (*cb)(bool, struct TdObject *, struct TdError *));

void fetal_cb(bool successful, struct TdObject *result, struct TdError *error);

int post_auth();

#endif /* _MUTEBOT_TDUTILS_H */