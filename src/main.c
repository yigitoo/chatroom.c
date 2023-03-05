#include "main.h"
struct nickname {
  size_t len;
  char nick[];
}

static struct nickname MISSING_NICKNAME = {.len = 7, .nick = "unknown"};
static FIOBJ CHAT_CHANNEL;
/* *****************************************************************************
Websocket callbacks
***************************************************************************** */

/* We'll subscribe to the channel's chat channel when a new connection opens */
static void on_open_websocket(ws_s *ws) {
  /* subscription - easy as pie */
  websocket_subscribe(ws, .channel = CHAT_CHANNEL, .force_text = 1);
  /* notify everyone about new (named) visitors */
  struct nickname *n = websocket_udata(ws);
  if (n) {
    FIOBJ msg = fiobj_str_new(n->nick, n->len);
    fiobj_str_write(msg, " joined the chat.", 17);
    pubsub_publish(.channel = CHAT_CHANNEL, .message = msg);
    /* cleanup */
    fiobj_free(msg);
  }
}

/* Free the nickname, if any. */
static void on_close_websocket(ws_s *ws) {
  struct nickname *n = websocket_udata(ws);
  if (n) {
    /* send notification */
    FIOBJ msg = fiobj_str_new(n->nick, n->len);
    fiobj_str_write(msg, " left the chat.", 15);
    pubsub_publish(.channel = CHAT_CHANNEL, .message = msg);
    /* cleanup */
    fiobj_free(msg);
    free(n);
  }
}

/* Received a message from a client, format message for chat . */
static void handle_websocket_messages(ws_s *ws, char *data, size_t size,
                                      uint8_t is_text) {
  struct nickname *n = websocket_udata(ws);
  if (!n)
    n = &MISSING_NICKNAME;

  /* allocates a dynamic string. knowing the buffer size is faster */
  FIOBJ msg = fiobj_str_buf(n->len + 2 + size);
  fiobj_str_write(msg, n->nick, n->len);
  fiobj_str_write(msg, ": ", 2);
  fiobj_str_write(msg, data, size);
  if (pubsub_publish(.channel = CHAT_CHANNEL, .message = msg))
    fprintf(stderr, "Failed to publish\n");
  fiobj_free(msg);
  (void)(ws);
  (void)(is_text);
}

/* *****************************************************************************
HTTP Handling (Upgrading to Websocket)
***************************************************************************** */

/* Answers simple HTTP requests */
static void answer_http_request(http_s *h) {
  http_set_header2(h, (fio_cstr_s){.name = "Server", .len = 6},
                   (fio_cstr_s){.value = "facil.example", .len = 13});
  http_set_header(h, HTTP_HEADER_CONTENT_TYPE, http_mimetype_find("txt", 3));
  /* this both sends the response and frees the http handler. */
  http_send_body(h, "This is a simple Websocket chatroom example.", 44);
}

/* tests that the target protocol is "websockets" and upgrades the connection */
static void answer_http_upgrade(http_s *h, char *target, size_t len) {
  /* test for target protocol name */
  if (len != 9 || memcmp(target, "websocket", 9)) {
    http_send_error(h, 400);
    return;
  }
  struct nickname *n = NULL;
  fio_cstr_s path = fiobj_obj2cstr(h->path);
  if (path.len > 1) {
    n = malloc(path.len + sizeof(*n));
    n->len = path.len - 1;
    memcpy(n->nick, path.data + 1, path.len); /* will copy the NUL byte */
  }
  // Websocket upgrade will use our existing response.
  if (http_upgrade2ws(.http = h, .on_open = on_open_websocket,
                      .on_close = on_close_websocket,
                      .on_message = handle_websocket_messages, .udata = n))
    free(n);
}

/* Our main function, we'll start up the server */
int main(void) {
  const char *port = "3000";
  const char *public_folder = NULL;
  uint32_t threads = 1;
  uint32_t workers = 1;
  uint8_t print_log = 0;
  CHAT_CHANNEL = fiobj_sym_new("chat", 4);

  if (http_listen(port, NULL, .on_request = answer_http_request,
                  .on_upgrade = answer_http_upgrade, .log = print_log,
                  .public_folder = public_folder) == -1)
    perror("Couldn't initiate Websocket service"), exit(1);
  facil_run(.threads = threads, .processes = workers);

  fiobj_free(CHAT_CHANNEL);
}