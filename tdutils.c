#include "mutebot.h"
#include "tdutils.h"
#include "handlers.h"

#include <stdio.h>
#include <td/telegram/td_c_client.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <limits.h>
#include <pthread.h>
#include <signal.h>
#include <stdatomic.h>

struct td_callback {
    struct td_callback *next;

    long long request_id;

    void (*cb)(bool, struct TdObject *, struct TdError *, void *);

    void *cb_arg;
};

int td = -1;
long long my_id = -1;
static struct td_callback *cbs;

/**
 * Last request_id. Increased whenever sending a query.
 *
 * Write: any
 * Read: any
 */
static atomic_llong last_req_id = 0;

/**
 * Set to true by main thread when received authorizationStateClosed or authorizationStateClosing.
 * Stop accepting new queries.
 *
 * Write: main
 * Read: any
 */
bool closing = false;

static bool sighandler_setup = false;
static pthread_t thread_sighandler;

/**
 * Used for sigwait(2).
 */
sigset_t set;

static void *main_sighandler(void *arg) {
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    int r;
    int sig;

    while (true) {
        r = sigwait(&set, &sig);
        if (r) {
            fprintf(stderr, "Cannot call sigwait(): %d.\n", r);
            goto cleanup;
        }
        switch (sig) {
            case SIGINT:
            case SIGTERM:
                if (td == -1) goto cleanup;
                td_send(TdCreateObjectLogOut(), NULL, NULL);
                goto cleanup;
            default:
                break;
        }
    }
    cleanup:
    pthread_exit(NULL);
}

static int sighandler_init() {
    int r;
    sigemptyset(&set);
    sigaddset(&set, SIGTERM);
    sigaddset(&set, SIGINT);
    sigaddset(&set, SIGUSR1);
    r = pthread_sigmask(SIG_BLOCK, &set, NULL);
    if (r) {
        fprintf(stderr, "Cannot call pthread_sigmask(): %d\n", r);
        goto cleanup;
    }
    r = pthread_create(&thread_sighandler, NULL, &main_sighandler, NULL);
    if (r) {
        fprintf(stderr, "Cannot call pthread_create(): %d\n", r);
        goto cleanup;
    }
    sighandler_setup = true;
    goto cleanup;
    cleanup:
    return r;
}

static int sighandler_close() {
    if (!sighandler_setup) return 0;
    pthread_cancel(thread_sighandler);
    int r = pthread_join(thread_sighandler, NULL);
    if (!r) sighandler_setup = false;
    return r;
}

static int tdcb_push(long long request_id, void (*cb)(bool, struct TdObject *, struct TdError *, void *), void *cb_arg) {
    struct td_callback *current_ptr = malloc(sizeof(struct td_callback));
    if (current_ptr == NULL) {
        int r = errno;
        fprintf(stderr, "Cannot allocate memory: %s\n", strerror(r));
        return r;
    }
    current_ptr->next = NULL;
    current_ptr->request_id = request_id;
    current_ptr->cb = cb;
    current_ptr->cb_arg = cb_arg;
    if (cbs == NULL) {
        cbs = current_ptr;
    } else {
        current_ptr->next = cbs;
        cbs = current_ptr;
    }
    return 0;
}

int td_send(void *func, void (*cb)(bool, struct TdObject *, struct TdError *, void *), void *cb_arg) {
    if (closing) {
        TdDestroyObjectFunction((struct TdFunction *)func);
        return 0;
    }
    if (last_req_id == LLONG_MAX) last_req_id = 0;
    last_req_id++;
    int r;
    if (cb != NULL && (r = tdcb_push(last_req_id, cb, cb_arg))) {
        return r;
    }
    TdCClientSend(td, (struct TdRequest) {
            last_req_id,
            (struct TdFunction *) func
    });
    return 0;
}

static int tdcb_call(long long request_id, bool successful, struct TdObject *result, struct TdError *error) {
    if (cbs == NULL) return 0;
    struct td_callback *current_ptr = cbs;
    bool node_found = false;
    while (current_ptr != NULL) {
        if (current_ptr->request_id == request_id) {
            node_found = true;
            current_ptr->cb(successful, result, error, current_ptr->cb_arg);
            if (error != NULL &&
                current_ptr->cb == &fetal_cb) {
                /* The fetal_cb callback does not print anything */
                /* because it does not know the request_id. */
                fprintf(stderr, "Error: Request %lld: %s (%d)\n",
                        request_id,
                        error->message_,
                        error->code_);
            }
            break;
        }
        current_ptr = current_ptr->next;
    }
    if (node_found) {
        /*
         * The callback function may insert nodes to the link.
         * Therefore, we do the iteration again after calling the function to
         * delete the current one.
         * Need a better implementation.
         */
        current_ptr = cbs;
        struct td_callback *prev_ptr = NULL;
        while (current_ptr != NULL) {
            if (current_ptr->request_id == request_id) {
                if (prev_ptr == NULL) cbs = current_ptr->next;
                else prev_ptr->next = current_ptr->next;
                free(current_ptr);
                return 0;
            }
            prev_ptr = current_ptr;
            current_ptr = current_ptr->next;
        }
    }
    if (error != NULL) {
        /* No callback found, Display default error message. */
        fprintf(stderr, "Error: Request %lld: %s (%d)\n",
                request_id,
                error->message_,
                error->code_);
    }
    return 0;
}

static void tdcb_free() {
    struct td_callback *current_ptr = cbs;
    while (current_ptr != NULL) {
        struct td_callback *bak = current_ptr;
        current_ptr = current_ptr->next;
        free(bak);
    }
    cbs = NULL;
}

int td_init() {
    int r;
    if ((r = sighandler_init())) return r;
    td = TdCClientCreateId();
    TdDestroyObjectObject(TdCClientExecute((struct TdFunction *) TdCreateObjectSetLogVerbosityLevel(0)));
    td_send(TdCreateObjectGetOption("version"), &fetal_cb, NULL);
    return 0;
}

void td_free() {
    sighandler_close();
    tdcb_free();
}

void tg_close() {
    if (td == -1) return;
    td_send(TdCreateObjectClose(), NULL, NULL);
}

void fetal_cb(bool successful, struct TdObject *result, struct TdError *error, void *cb_arg) {
    if (!successful) {
        tg_close();
    }
}

static void auth(bool successful, struct TdObject *result, struct TdError *error, void *cb_arg) {
    if (closing || successful) return;
    if (error != NULL) { /* error == NULL when caused from update handler */
        fprintf(stderr, "Invalid bot token or API ID / Hash: %s (%d)\n",
                error->message_,
                error->code_);
        tg_close();
        return;
    }
    td_send(TdCreateObjectCheckAuthenticationBotToken(cmd.bot_token), &auth, NULL);
}

static int handle_auth(const struct TdUpdateAuthorizationState *update) {
    switch (update->authorization_state_->ID) {
        case CODE_AuthorizationStateWaitTdlibParameters:
            td_send(TdCreateObjectSetTdlibParameters(TdCreateObjectTdlibParameters(
                            cmd.test_dc,
                            cmd.td_path,
                            NULL,
                            false,
                            false,
                            false,
                            false,
                            cmd.api_id,
                            cmd.api_hash,
                            "en",
                            "Desktop",
                            "0.0",
                            "MuteBot "VER_MAJOR"."VER_MINOR,
                            false,
                            true
                    )),
                    &fetal_cb,
                    NULL);
            return 0;
        case CODE_AuthorizationStateWaitPhoneNumber: {
            if (cmd.logout) {
                tg_close();
                return 0;
            }
            auth(false, NULL, NULL, NULL);
            return 0;
        }
        case CODE_AuthorizationStateReady: {
            if (cmd.logout) {
                td_send(TdCreateObjectLogOut(), &fetal_cb, NULL);
                return 0;
            }
            return post_auth();
        }
        case CODE_AuthorizationStateWaitEncryptionKey: {
            td_send(TdCreateObjectCheckDatabaseEncryptionKey(TdCreateObjectBytes((unsigned char *) {0x0}, 0)),
                    &fetal_cb, NULL);
            return 0;
        }
        case CODE_AuthorizationStateLoggingOut: {
            return 0;
        }
        /* Closed state is handled in the main loop. */
        case CODE_AuthorizationStateClosing: {
            closing = true;
            return 0;
        }
        case CODE_AuthorizationStateWaitOtherDeviceConfirmation: {
            struct TdAuthorizationStateWaitOtherDeviceConfirmation *waitOtherDeviceConfirmation =
                    (struct TdAuthorizationStateWaitOtherDeviceConfirmation *)update->authorization_state_;
            printf("Please scan the QR code of the following link using another Telegram seession:\n%s\n",
                   waitOtherDeviceConfirmation->link_);
            return 0;
        }
        default: {
            fprintf(stderr, "Unsupported authorization state: %d. Aborted.\n",
                    update->authorization_state_->ID);
            tg_close();
            return 0;
        }
    }
}

static int handle_option(const struct TdUpdateOption *update) {
    if (!strcmp("my_id", update->name_) &&
        update->value_->ID == CODE_OptionValueInteger) {
        const struct TdOptionValueInteger *integer =
                (struct TdOptionValueInteger *) update->value_;
        my_id = integer->value_;
    }
    return 0;
}

static int handle_update(const struct TdUpdate *update) {
    switch (update->ID) {
        case CODE_UpdateAuthorizationState:
            return handle_auth((struct TdUpdateAuthorizationState *) update);
        case CODE_UpdateNewMessage:
            return handle_message((struct TdUpdateNewMessage *) update);
        case CODE_UpdateOption:
            return handle_option((struct TdUpdateOption *) update);
        default:
            return 0;
    }
}

void td_loop() {
    struct TdResponse response;
    while (1) {
        response = TdCClientReceive(5);
        struct TdObject *obj = response.object;
        if (obj == NULL) continue;
        const bool is_update = response.request_id == 0;
        if (is_update &&
            obj->ID == CODE_UpdateAuthorizationState &&
            ((struct TdUpdate *) obj)->ID == CODE_UpdateAuthorizationState &&
            ((struct TdUpdateAuthorizationState *) obj)->authorization_state_->ID ==
            CODE_AuthorizationStateClosed) {
            closing = true;
            TdDestroyObjectObject(obj);
            return;
        }
        if (is_update) {
            handle_update((struct TdUpdate *) obj);
            TdDestroyObjectObject(obj);
            continue;
        }
        switch (obj->ID) {
            case CODE_Ok:
                tdcb_call(response.request_id, true, NULL, NULL);
                break;
            case CODE_Error: {
                struct TdError *error =
                        (struct TdError *) obj;
                tdcb_call(response.request_id, false, NULL, error);
                break;
            }
            default:
                tdcb_call(response.request_id, true, obj, NULL);
                break;
        }
        TdDestroyObjectObject(obj);
    }
}
