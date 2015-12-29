#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

#include "server.h"
#include "rules_parser.h"

struct app_args {
  char *rules_file_name;
};

struct app_args process_app_arguments(int argc, char *argv[]) {
  if (argc < 2) {
    fprintf(stderr, "Not enough arguments supplied.\n");
    exit(EXIT_FAILURE);
  }
  int option_index = 0;
  int getopt_long_result;
  struct app_args args;
  struct option long_options[] = {
    {"rules_file", required_argument, 0, 'r'},
    {0,            0, 0,    0}}
  ;
  while((getopt_long_result = getopt_long(argc, argv, "r:", long_options, &option_index)) != -1) {
    switch(getopt_long_result) {
    case 'r':
      args.rules_file_name = optarg;
      break;
    }
  }
  return args;
}

int main(int argc, char *argv[]) {
  struct app_args args = process_app_arguments(argc, argv);
  char *rules_string = read_rules_file(args.rules_file_name);
  struct rule_message_t *rule_messages = parse_rules(rules_string);
  server_loop(rule_messages);
};
