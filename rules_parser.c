#include <stdio.h>
#include <stdlib.h>

#include "http.h"
#include "rules_parser.h"


char *read_rules_file(const char *path) {
  FILE *file = fopen(path, "r");
  if (!file) {
    fprintf(stderr, "Couldn't open a rules file\n");
    perror("read_rules_file");
    exit(EXIT_FAILURE);
  }

  fseek(file, 0, SEEK_END);
  long file_size = ftell(file);
  rewind(file);
  char *file_content = malloc(file_size + 1);
  fread(file_content, file_size, 1, file);
  fclose(file);
  file_content[file_size] = 0;

  return file_content;
};
}
