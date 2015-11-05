#ifndef RULES_PARSER_H
#define RULES_PARSER_H

#include <stdio.h>
#include "http.h"

char* read_rules_file(const char* path);
struct rule_message_t *parse_rules(char *rules_string);

struct rule_request_t {
  struct http_request_t *super;
  const char *identifier;
  struct rule_request_t *inherited_from;
};

struct rule_response_t {
  struct http_response_t *super;
  const char *identifier;
  struct rule_response_t *inherited_from;
};

struct rule_message_t {
  struct rule_request_t *request;
  struct rule_response_t *response;
  struct rule_message_t *next;
};

#endif
