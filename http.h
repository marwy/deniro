#ifndef HTTP_H
#define HTTP_H

#include <stdint.h>

struct http_request_t *http_request_new(void);
struct http_response_t *http_response_new(void);
enum HTTP_METHOD http_method_string_to_enum(char *string);
char *http_response_to_string(struct http_response_t *response);
struct http_header_t *add_header(struct http_header_t *headers, char *key, char *value);
struct http_request_t *parse_http_request(char *buffer);
struct http_header_t *get_last_http_header(struct http_header_t *headers_start);
void copy_http_request(struct http_request_t *dest, struct http_request_t *src);
void copy_http_headers(struct http_header_t **dest, struct http_header_t *src);
void copy_http_response(struct http_response_t *dest, struct http_response_t *src);

enum HTTP_METHOD {
  NO_METHOD,
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
