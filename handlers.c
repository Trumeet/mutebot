#include "mutebot.h"
#include "tdutils.h"

#include <stdio.h>

static void restrict_user(long long chat_id, long long uid) {
    struct TdChatPermissions *permissions =
            TdCreateObjectChatPermissions(0,
                                          0,
                                          0,
                                          0,
                                          0,
                                          0,
                                          0,
                                          0);
    struct TdChatMemberStatus *status =
            (struct TdChatMemberStatus *) TdCreateObjectChatMemberStatusRestricted(1,
                                                                                   0,
                                                                                   permissions);
    td_send(TdCreateObjectSetChatMemberStatus(chat_id,
                                              (struct TdMessageSender *) TdCreateObjectMessageSenderUser(uid),
                                              status),
            NULL,
            NULL);
}

int handle_message(struct TdUpdateNewMessage *update) {
    const struct TdMessage *message = update->message_;
    if (message->sender_->ID == CODE_MessageSenderUser &&
        (message->content_->ID == CODE_MessageChatJoinByRequest ||
         message->content_->ID == CODE_MessageChatJoinByLink)) {
        restrict_user(message->chat_id_, ((struct TdMessageSenderUser *) message->sender_)->user_id_);
        return 0;
    }
    if (message->content_->ID != CODE_MessageChatAddMembers)
        return 0;
    struct TdMessageChatAddMembers *addMembers =
            (struct TdMessageChatAddMembers *) message->content_;
    for (int i = 0; i < addMembers->member_user_ids_->len; i++) {
        const long long uid = addMembers->member_user_ids_->data[i];
        restrict_user(message->chat_id_, uid);
    }
    return 0;
}