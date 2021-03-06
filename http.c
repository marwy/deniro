#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdbool.h>

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
    for(size_t i = 0; i < strlen(string); i++) {
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

size_t header_length_including_whitespace(char *key, char *value) {
  size_t length = 0;
  length += strlen(key);
  length += 1; // colon
  length += 1; // space
  length += strlen(value);
  length += 2; // CRLF
  return length;
};

char *http_response_to_string(struct http_response_t *response) {
  size_t headers_length = 0;
  size_t body_length = 0;
  struct http_header_t *temp_header;

  const char *http_version = response->status_line->http_version;
  if (!http_version)
    http_version = "HTTP/1.1";
  char *status_code = malloc(5); // max of uint16
  sprintf(status_code, "%d", response->status_line->status_code);
  const char *reason_phrase = response->status_line->reason_phrase;
  // include space for spaces and CRLF at the end
  size_t status_line_length = strlen(status_code) + 1 + strlen(reason_phrase) + 1 +
    strlen(http_version) + 2;

  temp_header = response->headers;
  while(temp_header) {
    headers_length += header_length_including_whitespace(temp_header->name,
                                                         temp_header->value);
    temp_header = temp_header->next_header;
  }

  if (response->body) {
    body_length = strlen(response->body);

    temp_header = response->headers;
    bool found = false;
    while(temp_header) {
      if (strcmp(temp_header->name, "Content-Length") == 0) {
        found = true;
        break;
      }
      temp_header = temp_header->next_header;
    }
    if (!found) {
      char body_length_as_string[1024];
      snprintf(body_length_as_string, 1024, "%zd", body_length);
      response->headers = add_header(response->headers, "Content-Length",
                                     body_length_as_string);
      headers_length += header_length_including_whitespace("Content-Length",
                                                           body_length_as_string);
    }
  }

  //includes CRLF before the body
  size_t response_length = status_line_length + headers_length + 2 + body_length;
  char *buffer = malloc(response_length + 1);

  snprintf(buffer, response_length, "%s %s %s\r\n", http_version, status_code, reason_phrase);
  temp_header = response->headers;
  while(temp_header) {
    snprintf(buffer + strlen(buffer), response_length, "%s: %s\r\n",
            temp_header->name, temp_header->value);
    temp_header = temp_header->next_header;
  }
  snprintf(buffer + strlen(buffer), response_length, "\r\n");
  if (response->body)
    snprintf(buffer + strlen(buffer), response_length, "%s", response->body);
  return buffer;
}

struct http_header_t *add_header(struct http_header_t *headers, char *key, char *value) {
  char *header_name = strdup(key);
  char *header_value = strdup(value);
  if (!headers) {
    headers = calloc(1, sizeof(struct http_header_t));
    headers->name = header_name;
    headers->value = header_value;
  }
  else {
    struct http_header_t *temp_header = headers;
    while (1) {
      if (strcmp(temp_header->name, header_name) == 0) {
        temp_header->value = header_value;
        break;
      }
      if (!temp_header->next_header) {
        struct http_header_t *next_header = calloc(1, sizeof(struct http_header_t));
        next_header->name = header_name;
        next_header->value = header_value;
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
    char *header_key = malloc(header_key_length + 1);
    for (size_t i = 0; i < header_key_length; i++) {
      header_key[i] = buffer[i + header_key_start];
    }
    header_key[header_key_length] = '\0';

    size_t header_value_start = buffer_index + 2; // + 2 to move past the colon and space
    while (buffer[buffer_index] != '\r')
      buffer_index++;
    size_t header_value_end = buffer_index++;
    size_t header_value_length = header_value_end - header_value_start;
    char *header_value = malloc(header_value_length + 1);
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

struct http_header_t *get_last_http_header(struct http_header_t *first_header) {
  struct http_header_t *last_header = first_header;
  if (!last_header)
    return last_header; // aint nobody got time for babysitting
  while (last_header->next_header)
    last_header = last_header->next_header;
  return last_header;
}

void copy_http_headers(struct http_header_t **dest_hdr,
                            struct http_header_t *source_hdr) {

  struct http_header_t *temp_source_header = source_hdr;
  struct http_header_t *temp_dest_header = *dest_hdr;
  struct http_header_t *last_header = get_last_http_header(temp_dest_header);
  while(temp_source_header) {
    bool found = false;
    while(temp_dest_header) {
      // note that we only care about the name of the header, its value equality
      // should be checked when adding a header in parsing phase
      if (strcmp(temp_dest_header->name, temp_source_header->name) == 0) {
        found = true;
        break;
      } else
        temp_dest_header = temp_dest_header->next_header;
    }
    if (!found) {
      struct http_header_t *temp_header = malloc(sizeof(struct http_header_t));
      temp_header->name = strdup(temp_source_header->name);
      temp_header->value = strdup(temp_source_header->value);
      if (!*dest_hdr)
        *dest_hdr = temp_header;
      if (!last_header)
        last_header = temp_header;
      else {
        last_header->next_header = temp_header;
        last_header = last_header->next_header;
      }
    }
    temp_source_header = temp_source_header->next_header;
  }
}

void copy_http_request(struct http_request_t *dest, struct http_request_t *src) {
  if (!dest->request_line->method)
    dest->request_line->method = src->request_line->method;
  if (!dest->request_line->url)
    dest->request_line->url = src->request_line->url;
  if (!dest->request_line->http_version)
    dest->request_line->http_version = src->request_line->http_version;

  if (!dest->body)
    dest->body = src->body;

  copy_http_headers(&dest->headers, src->headers);
}

void copy_http_response(struct http_response_t *dest, struct http_response_t *src) {
  if (!dest->status_line->status_code)
    dest->status_line->status_code = src->status_line->status_code;
  if (!dest->status_line->reason_phrase)
    dest->status_line->reason_phrase = src->status_line->reason_phrase;
  if (!dest->status_line->http_version)
    dest->status_line->http_version = src->status_line->http_version;

  if (!dest->body)
    dest->body = src->body;

  copy_http_headers(&dest->headers, src->headers);
}
