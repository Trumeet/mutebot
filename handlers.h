#ifndef _MUTEBOT_HANDLERS_H
#define _MUTEBOT_HANDLERS_H

#include <td/telegram/td_c_client.h>

int handle_message(struct TdUpdateNewMessage *update);

#endif /* _MUTEBOT_HANDLERS_H */
