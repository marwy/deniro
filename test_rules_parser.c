#include "deniro_assert.h"
#include "rules_parser.h"

void test_rules_parser() {
  struct rule_message_t *next_rule;
  char *rules_string = \
    "[req]\n"
    "url = /contact/delete\n"
    "method = DELETE\n"
    "name = generic contact req\n"
    "[res]\n"
    "body = response body\n"
    "name = generic contact res\n"
    "[req]\n"
    "url = /search/\n"
    "method = PUT\n"
    "User-Agent = All Hail Cthulhu 3/2.1\n"
    "[res]\n"
    "body = this is what i found\n"
    "Tracking-ID = 0xdeadbeef\n"
    "[req]\n"
    "inherit = generic contact req\n"
    "url = /contact/post\n"
    "method = POST\n"
    "body = subject=to hell with c\n"
    "[res]\n"
    "inherit = generic contact res\n"
    "body = response\n"
    "status = 200\n";

  struct rule_message_t *rules = parse_rules(rules_string);
  den_assert_str_eq(rules->request->super->request_line->url,
                    "/contact/delete");
  den_assert(rules->request->super->request_line->method == DELETE);
  den_assert_str_eq(rules->request->identifier, "generic contact req");
  den_assert_str_eq(rules->response->super->body, "response body");
  den_assert_str_eq(rules->response->identifier, "generic contact res");
  den_assert(rules->request->accuracy == 2);
  den_assert(rules->response->accuracy == 1);

  next_rule = rules->next;
  den_assert_str_eq(next_rule->request->super->request_line->url,
                    "/search/");
  den_assert(next_rule->request->super->request_line->method == PUT);
  den_assert_str_eq(next_rule->request->super->headers->name, "User-Agent");
  den_assert_str_eq(next_rule->request->super->headers->value, "All Hail Cthulhu 3/2.1");
  den_assert_str_eq(next_rule->response->super->body, "this is what i found");
  den_assert_str_eq(next_rule->response->super->headers->name, "Tracking-ID");
  den_assert_str_eq(next_rule->response->super->headers->value, "0xdeadbeef");
  den_assert(next_rule->request->accuracy == 3);
  den_assert(next_rule->response->accuracy == 2);

  next_rule = next_rule->next;
  den_assert_str_eq(next_rule->request->super->request_line->url,
                    "/contact/post");
  den_assert(next_rule->request->super->request_line->method == POST);
  den_assert_str_eq(next_rule->request->super->body,
                    "subject=to hell with c");
  den_assert_str_eq(next_rule->response->super->body, "response");
  den_assert(next_rule->request->accuracy == 3);
  den_assert(next_rule->response->accuracy == 2);
}

void rules_parser_test_suite(void) {
  RUN_TEST("test_rules_parser", test_rules_parser);
}
