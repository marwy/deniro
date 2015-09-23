#ifndef RULES_PARSER_H
#define RULES_PARSER_H

#include <stdio.h>

char* read_rules_file(const char* path);

typedef struct http_message_t rule_request_t;
typedef struct http_message_t rule_response_t;

struct rule_message_t {
  rule_request_t *request;
  rule_response_t *response;
  struct rule_message_t *next;
};

#endif
