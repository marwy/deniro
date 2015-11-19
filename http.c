#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>

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

enum HTTP_METHOD http_method_string_to_enum(char *string) {
  if (islower(string[0])) {
    for(int i = 0; i < strlen(string); i++) {
      string[i] = toupper(string[i]);
    };
  }
  if (strcmp(string, "HEAD") == 0)
    return HEAD;
  else if (strcmp(string, "GET") == 0)
    return GET;
  else if (strcmp(string, "POST") == 0)
    return POST;
  else if (strcmp(string, "PUT") == 0)
    return PUT;
  else if (strcmp(string, "OPTIONS") == 0)
    return OPTIONS;
  else if (strcmp(string, "DELETE") == 0)
    return DELETE;
  else {
    fprintf(stderr, "Couldn't match value: %s with any HTTP method, falling back to default GET\n", string);
    return GET;
  }
};
