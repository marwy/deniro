#include <stdlib.h>

#include "http.h"

struct http_request_t *http_request_new(void) {
  struct http_request_t *request = calloc(1, sizeof(struct http_request_t));
  request->request_line = calloc(1, sizeof(struct http_request_line_t));
  return request;
};

struct http_response_t *http_response_new(void) {
  struct http_response_t *response = calloc(1, sizeof(struct http_request_t));
  response->status_line = calloc(1, sizeof(struct http_status_line_t));
  return response;
};
