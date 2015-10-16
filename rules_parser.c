#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <stdint.h>

#include "http.h"
#include "rules_parser.h"

#define SECTION_NAME_LENGTH 3


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

int advance_to_next_line(char *rules_string, int *begin, int *end) {
  size_t rules_string_len = strlen(rules_string);
  if (!*end && (*end >= strlen(rules_string)))
      return -1;
  *begin = *end + 1;
  (*end)++; // move past the previous newline
  if (*end >= rules_string_len)
    return -1;
  while (rules_string[*end] && rules_string[*end] != '\n') {
    *end += 1;
  };
  return 0;
}

void parse_section(char *rules_string, int line_begin, char *section_name) {
  for (int i = 0; i < SECTION_NAME_LENGTH; i++) {
    section_name[i] = rules_string[line_begin + 1 + i];
  };
  section_name[SECTION_NAME_LENGTH] = '\0';
};

char *parse_key(char *rules_string, int line_begin, int line_end, int *equal_sign_index) {
  int line_begin_temp = line_begin;
  for (; line_begin_temp < line_end; line_begin_temp++) {
    if (rules_string[line_begin_temp] == '=')
      break;
  };
  size_t key_length = (line_begin_temp - line_begin) - 1;
  char *key = malloc(key_length);
  for (int i = 0; i < key_length; i++) {
    char ch = rules_string[line_begin + i];
    if (!isspace(ch))
      key[i] = ch;
  };
  key[key_length] = '\0';
  *equal_sign_index = line_begin_temp;
  return key;
};

char *parse_value(char *rules_string, int *line_begin, int *line_end, int value_pos) {
  size_t value_length = 0;
  uint8_t multiline_value = 0;
  while (1) {
    char curr_char = rules_string[value_pos];
    if (isspace(curr_char))
      value_pos++;
    else {
      if (curr_char == '"') {
        multiline_value = 1;
      }
      break;
    }
  }
  if (multiline_value) {
    do  {
      advance_to_next_line(rules_string, line_begin, line_end);
    } while (!(rules_string[*line_begin] == '"'));
    value_pos++; // ignore the starting "
    value_length = ((*line_end - 1) - value_pos); //ignores the trailing "
  }
  else {
    value_length = (*line_end - value_pos);
  }
  char *value = malloc(value_length);
  for (int i = 0, si = 0; i < value_length; i++, si++) {
    char ch = rules_string[value_pos + i];
    value[si] = ch;
  };
  value[value_length] = '\0';
  return value;
};


struct rule_message_t *parse_rules(char *rules_string) {
  int current_line_begin = 0;
  // new beginning is basically an end + 1, so (-1) + 1 gets us a real beginning for the first line
  int current_line_end = -1;
  char section_name[3];
  struct rule_message_t *rule_message = malloc(sizeof(struct rule_message_t));
  struct rule_message_t *current_message;

  while (advance_to_next_line(rules_string, &current_line_begin, &current_line_end) != -1) {

    if (rules_string[current_line_begin] == '[') {
      parse_section(rules_string, current_line_begin, section_name);

      advance_to_next_line(rules_string, &current_line_begin, &current_line_end);

      if (strcmp(section_name, "req") == 0) {
        current_message = malloc(sizeof(struct rule_message_t));
        while (rules_string[current_line_begin] != '[') {

          int key_end_index = 0;
          char *key = parse_key(rules_string, current_line_begin, current_line_end, &key_end_index);
          printf("the key is %s\n", key);

          key_end_index++; // move past the equal sign

          char *value = parse_value(rules_string, &current_line_begin, &current_line_end, key_end_index);

          printf("the value is %s\n", value);

          #define IDENT_MATCH(k) strcmp(key, k) == 0
          if (IDENT_MATCH("url")) {
            current_message->request->url = value;
          } else if (IDENT_MATCH("name")) {
            current_message->request->name = value;
          } else if (IDENT_MATCH("method")) {
            /* current_message->request->method = value; */
            // TODO: method is an enum, value is a char *, so we need to map the method string to enum value
          }
          advance_to_next_line(rules_string, &current_line_begin, &current_line_end);
        };
      }
      else if (strcmp(section_name, "res") == 0) {
        printf("response type!\n");
      };
    };
  };
  return (rule_message);
};
