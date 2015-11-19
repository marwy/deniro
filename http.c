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

struct http_header_t *add_header(struct http_header_t *headers, char *key, char *value) {
  char *header_name = calloc(1, strlen(key) + 1);
  strcpy(header_name, key);
  if (!headers) {
    headers = calloc(1, sizeof(struct http_header_t));
    headers->name = header_name;
    headers->value = value;
  }
  else {
    struct http_header_t *temp_header = headers;
    while (1) {
      if (!temp_header->next_header) {
        struct http_header_t *next_header = calloc(1, sizeof(struct http_header_t));
        next_header->name = header_name;
        next_header->value = value;
        temp_header->next_header = next_header;
        break;
      }
      else
        temp_header = temp_header->next_header;
    }
  }
  return headers;
};

struct http_request_t *parse_http_request(char *buffer) {
  struct http_request_t *request = http_request_new();
  size_t buffer_index = 0;
  size_t buffer_length = strlen(buffer);
  uint32_t content_length = 0;

  // method parsing
  while (buffer[buffer_index] != ' ' && buffer_index < buffer_length) {
    buffer_index++;
  }
  size_t method_length = buffer_index;
  char *method_string = malloc(method_length + 1);
  for (size_t i = 0; i < method_length; i++) {
    method_string[i] = buffer[i];
  }
  method_string[method_length] = '\0';
  enum HTTP_METHOD method = http_method_string_to_enum(method_string);
  request->request_line->method = method;

  // url parsing
  buffer_index++;
  while (buffer[buffer_index] != ' ' && buffer_index < buffer_length) {
    buffer_index++;
  }
  size_t url_start = method_length + 1;
  size_t url_length = buffer_index - url_start;
  char *url = malloc(url_length + 1);
  for (size_t i = 0; i < url_length; i++) {
    url[i] = buffer[i + url_start];
  }
  url[url_length] = '\0';
  request->request_line->url = url;

  // version parsing
  size_t version_start = url_length + url_start + 1;
  buffer_index = version_start;
  while (buffer[buffer_index] != '\r' && buffer_index < buffer_length) {
    buffer_index++;
  }
  size_t version_length = buffer_index - version_start;
  char *version = malloc(version_length + 1);
  for (size_t i = 0; i < version_length; i++) {
    version[i] = buffer[i + version_start];
  }
  version[version_length] = '\0';
  request->request_line->http_version = version;

  // headers parsing
  if (buffer_index + 2 >= buffer_length)
    return request; // no headers? that's an invalid request but meh
  else {
    buffer_index += 2; // move past the \r\n that finishes request line
  }

  while (buffer[buffer_index] != '\r' && buffer[buffer_index + 1] != '\n') {
    size_t header_key_start = buffer_index;
    while(buffer[buffer_index] != ':' && buffer_index < buffer_length)
      buffer_index++;
    size_t header_key_length = buffer_index - header_key_start;
    char *header_key = malloc(header_key_length);
    for (size_t i = 0; i < header_key_length; i++) {
      header_key[i] = buffer[i + header_key_start];
    }
    header_key[header_key_length] = '\0';

    size_t header_value_start = buffer_index + 2; // + 2 to move past the colon and space
    while (buffer[buffer_index] != '\r')
      buffer_index++;
    size_t header_value_end = buffer_index++;
    size_t header_value_length = header_value_end - header_value_start;
    char *header_value = malloc(header_value_length);
    for (size_t i = 0; i < header_value_length; i++) {
      header_value[i] = buffer[i + header_value_start];
    }
    header_value[header_value_length] = '\0';

    request->headers = add_header(request->headers, header_key, header_value);

    if (strcmp(header_key, "Content-Length") == 0) {
      content_length = atoi(header_value);
    }

    buffer_index++; // move past the \n
  }

  // body parsing
  char index_exceeds_buffer = (buffer_index + 3) > buffer_length;
  if (!index_exceeds_buffer) {
    buffer_index += 2; // move past the \r\n
    char *body = malloc(content_length);
    for (size_t i = 0; i < content_length; i++) {
      body[i] = buffer[i + buffer_index];
    }
    body[content_length] = '\0';
    request->body = body;
  }

  return request;
};
