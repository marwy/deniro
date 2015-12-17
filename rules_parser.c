#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <stdint.h>

#include "rules_parser.h"
#include "http.h"

#define SECTION_NAME_LENGTH 3
#define KEY_MATCH(k) strcmp(key, k) == 0

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
  if (line_begin_temp == line_end) {
    fprintf(stderr, "We've reached the end of the line and still couldn't find '='\n");
    fprintf(stderr, "Please fix your config \n");
    exit(EXIT_FAILURE);
  }
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

struct rule_message_t *rule_message_new(void) {
  // Note that the _headers_ field is actually malloc'ed later on (if needed)
  struct rule_message_t *current_message = calloc(1, sizeof(struct rule_message_t));

  current_message->request = calloc(1, sizeof(struct rule_request_t));
  struct http_request_t *request = http_request_new();
  current_message->request->super = request;

  current_message->response = calloc(1, sizeof(struct rule_response_t));
  struct http_response_t *response = http_response_new();
  current_message->response->super = response;

  return current_message;
};

struct rule_message_t *parse_rules(char *rules_string) {
  int current_line_begin = 0;
  // new beginning is basically an end + 1, so (-1) + 1 gets us a real beginning for the first line
  int current_line_end = -1;
  char section_name[3];
  struct rule_message_t *current_message = {0};
  struct rule_message_t *rule_message = NULL;
  struct rule_message_t *previous_message = NULL;

  while (advance_to_next_line(rules_string, &current_line_begin, &current_line_end) != -1 && (current_line_begin < strlen(rules_string))) {

    if (rules_string[current_line_begin] == '[') {
      parse_section(rules_string, current_line_begin, section_name);

      advance_to_next_line(rules_string, &current_line_begin, &current_line_end);

      if (strcmp(section_name, "req") == 0) {
        current_message = rule_message_new();
      }


      while (rules_string[current_line_begin] != '[' && !isspace(rules_string[current_line_begin]) && (current_line_begin < strlen(rules_string))) {
        int key_end_index = 0;
        char *key = parse_key(rules_string, current_line_begin, current_line_end, &key_end_index);

        key_end_index++; // move past the equal sign

        char *value = parse_value(rules_string, &current_line_begin, &current_line_end, key_end_index);

        if (strcmp(section_name, "req") == 0) {
          struct http_request_t *request = current_message->request->super;
          if (KEY_MATCH("inherit")) {
            struct rule_message_t *temp_message = rule_message;
            while(temp_message) {
              if (strcmp(temp_message->request->identifier, value) == 0) {
                copy_http_request(current_message->request->super, temp_message->request->super);
                current_message->request->inherited_from = temp_message->request;
                break;
              }
              else
                temp_message = temp_message->next;
            }
          } else if (KEY_MATCH("name")) {
            current_message->request->identifier = value;
          } else if (KEY_MATCH("method")) {
            enum HTTP_METHOD method = http_method_string_to_enum(value);
            request->request_line->method = method;
            current_message->request->accuracy += 1;
          } else if (KEY_MATCH("url")) {
            request->request_line->url = value;
            current_message->request->accuracy += 1;
          }  else if (KEY_MATCH("body")) {
            request->body = value;
            current_message->request->accuracy += 1;
          }  else {
            // most likely we got ourselves a header
            request->headers = add_header(request->headers, key, value);
            current_message->request->accuracy += 1;
          }

        } else if (strcmp(section_name, "res") == 0) {
          struct http_response_t *response = current_message->response->super;
          if (KEY_MATCH("inherit")) {
            struct rule_message_t *temp_message = rule_message;
            while(temp_message) {
              if (strcmp(temp_message->response->identifier, value) == 0) {
                copy_http_response(current_message->response->super, temp_message->response->super);
                current_message->response->inherited_from = temp_message->response;
                break;
              }
              else
                temp_message = temp_message->next;
            }
          } else if (KEY_MATCH("name")) {
            current_message->response->identifier = value;
          } else if (KEY_MATCH("status_code") || KEY_MATCH("status")) {
            int status_code = atoi(value);
            if (status_code) {
              response->status_line->status_code = status_code;
            }
            else
              fprintf(stderr, "Couldn't parse value: %s to integer\n", value);
          }  else if (KEY_MATCH("phrase")) {
            response->status_line->reason_phrase = value;
          } else if (KEY_MATCH("body")) {
            response->body = value;
          } else {
            // most likely we got ourselves a header
            response->headers = add_header(response->headers, key, value);
          }

        };
        free(key);
        advance_to_next_line(rules_string, &current_line_begin, &current_line_end);
      };

      // undo advancing to the next line, otherwise we would forget the section
      current_line_end = current_line_begin - 1;
      if (strcmp(section_name, "req") == 0) {
        if (!previous_message)
          previous_message = current_message;
        else {
          previous_message->next = current_message;
          previous_message = previous_message->next;
        }

        // setting just the rule_message as a head
        if (!rule_message) {
          rule_message = previous_message;
        }
        else if (!rule_message->next) {
          rule_message->next = previous_message;
        }
      }
    };
  };

  return rule_message;
};
