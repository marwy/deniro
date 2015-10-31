#ifndef HTTP_H
#define HTTP_H

#include <stdint.h>

enum HTTP_METHOD {
  HEAD,
  GET,
  POST,
  PUT,
  OPTIONS,
  DELETE
};

struct http_header_t {
  char *name;
  char *value;
  struct http_header_t *next_header;
};

struct http_request_line_t {
  enum HTTP_METHOD method;
  const char *url;
  const char *http_version;
};

struct http_request_t {
  struct http_request_line_t *request_line;
  struct http_header_t *headers;
  const char *body;
};

struct http_status_line_t {
  const char *http_version;
  uint16_t      status_code;
  const char *reason_phrase;
};

struct http_response_t {
  struct http_status_line_t *status_line;
  struct http_header_t *headers;
  const char *body;
};
#endif
