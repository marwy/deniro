#ifndef HTTP_H
#define HTTP_H

enum HTTP_METHOD {
  HEAD,
  GET,
  POST,
  PUT,
  OPTIONS,
  DELETE
};

struct http_message_header {
  char *name;
  char *value;
  struct http_message_header *next_header;
};

struct http_message_t {
  const char *name;
  const char *url;
  enum HTTP_METHOD method;
  const char *body;
  struct http_message_header *header_start;
};
#endif
