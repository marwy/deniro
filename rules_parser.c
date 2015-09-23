#include <stdio.h>
#include <stdlib.h>

#include "http.h"
#include "rules_parser.h"


FILE* read_rules_file(const char *path) {
  FILE *file = fopen(path, "r");
  if (!file) {
    fprintf(stderr, "Couldn't open a rules file\n");
    perror("read_rules_file");
    exit(EXIT_FAILURE);
  }
  return file;
}
