#include <stdio.h>
#include <stdlib.h>

#include "http.h"


typedef struct http_message_t rule_request_t;

typedef struct http_message_t rule_response_t;

struct rule_message_t {
  rule_request_t *request;
  rule_response_t *response;
  struct rule_message_t *next;
};

FILE* read_rules_file(const char *path) {
  FILE *file = fopen(path, "r");
  if (!file) {
    fprintf(stderr, "Couldn't open a rules file\n");
    perror("read_rules_file");
    exit(EXIT_FAILURE);
  }
  return file;
}
