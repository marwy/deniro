#include "deniro_assert.h"
#include "rules_parser.h"

struct rule_message_t **add_to_matches(struct rule_message_t *matches[], size_t *matches_length, struct rule_message_t *message, ssize_t *index_last_written_to);
struct rule_message_t **collect_matching_rules_for_request(struct http_request_t *parsed_request, struct rule_message_t *rule_messages, size_t *matching_rules_length);
struct rule_message_t *get_best_matching_rule(struct rule_message_t *matching_rules[], size_t rules_length);

void test_add_to_matches(void) {
  size_t matches_reserved_capacity = 2;
  struct rule_message_t **matches = calloc(matches_reserved_capacity, sizeof(struct rule_message_t *));
  ssize_t index_last_written_to = -1;

  struct rule_message_t *first_message = malloc(sizeof(struct rule_message_t));
  first_message->request = malloc(sizeof(struct rule_request_t));
  first_message->request->identifier = "hello world one";
  matches = add_to_matches(matches, &matches_reserved_capacity, first_message, &index_last_written_to);

  struct rule_message_t *second_message = malloc(sizeof(struct rule_message_t));
  second_message->request = malloc(sizeof(struct rule_request_t));
  second_message->request->identifier = "hello world two";
  matches = add_to_matches(matches, &matches_reserved_capacity, second_message, &index_last_written_to);

  struct rule_message_t *third_message = malloc(sizeof(struct rule_message_t));
  third_message->request = malloc(sizeof(struct rule_request_t));
  third_message->request->identifier = "hello world three";
  matches = add_to_matches(matches, &matches_reserved_capacity, third_message, &index_last_written_to);

  struct rule_message_t *fourth_message = malloc(sizeof(struct rule_message_t));
  fourth_message->request = malloc(sizeof(struct rule_request_t));
  fourth_message->request->identifier = "hello world four";
  matches = add_to_matches(matches, &matches_reserved_capacity, fourth_message, &index_last_written_to);


  den_assert_str_eq(matches[0]->request->identifier, "hello world one");
  den_assert_str_eq(matches[1]->request->identifier, "hello world two");
  den_assert_str_eq(matches[2]->request->identifier, "hello world three");
  den_assert_str_eq(matches[3]->request->identifier, "hello world four");
}

void test_collect_matching_rules_for_request(void) {
  struct rule_message_t **matching_rules, *rule_messages, *first_rule,
    *second_rule, *third_rule;
  struct http_request_t *parsed_request = http_request_new();
  parsed_request->request_line->url = "/contact/";
  parsed_request->request_line->method = POST;
  parsed_request->headers = calloc(1, sizeof(struct http_header_t *));
  parsed_request->headers = &(struct http_header_t) {.name="User-Agent", .value="Mozilla Firefox 333"};
  parsed_request->headers->next_header = calloc(1, sizeof(struct http_header_t));
  parsed_request->headers->next_header = &(struct http_header_t)
    {.name="Referer", .value="www.example.org"};

  // turns out creating rules from string is easier than creating them by hand
  char *rules_string = \
    "[req]\n"               // this matches (url and header)
    "url = /contact/\n"
    "name = generic contact req\n"
    "User-Agent = Mozilla Firefox 333\n"
    "[res]\n"
    "body = response body\n"
    "name = generic contact res\n"
    //----
    "[req]\n"               // this doesn't because of the url
    "inherit = generic contact req\n"
    "url = /contact/aliens/\n"
    "method = POST\n"
    "name = aliens post\n"
    "[res]\n"
    "inherit = generic contact res\n"
    //-----
    "[req]\n"               // this doesn't because of the last header
    "method = DELETE\n"
    "inherit = generic contact req\n"
    "User-Agent = Google Chrome\n"
    "Referer = different-example.org\n"
    "name = delete google chrome\n"
    "[res]\n"
    "inherit = generic contact res\n"
    //-----
    "[req]\n"               // this matches (url, method and header)
    "inherit = generic contact req\n"
    "method = POST\n"
    "[res]\n"
    "inherit = generic contact res\n"
    //-----
    "[req]\n"              // this matches (solely Referer header)
    "Referer = www.example.org\n"
    "[res]\n"
    "inherit = generic contact res\n";
  rule_messages = parse_rules(rules_string);

  size_t matching_rules_length = 0;
  matching_rules = collect_matching_rules_for_request(parsed_request,
                                                      rule_messages,
                                                      &matching_rules_length);

  den_assert(matching_rules_length == 3);

  first_rule = matching_rules[0];
  den_assert_str_eq(first_rule->request->identifier,
                     "generic contact req");
  den_assert_str_eq(first_rule->request->super->request_line->url,
                     "/contact/");
  den_assert(first_rule->request->super->request_line->method == NO_METHOD);
  den_assert_str_eq(first_rule->request->super->headers->name,
                     "User-Agent");
  den_assert_str_eq(first_rule->request->super->headers->value,
                     "Mozilla Firefox 333");

  second_rule = matching_rules[1];
  den_assert_str_eq(second_rule->request->super->request_line->url,
                     "/contact/");
  den_assert(second_rule->request->super->request_line->method == POST);
  den_assert_str_eq(second_rule->request->super->headers->name,
                     "User-Agent");
  den_assert_str_eq(second_rule->request->super->headers->value,
                     "Mozilla Firefox 333");

  third_rule = matching_rules[2];
  den_assert(!third_rule->request->super->request_line->url);
  den_assert(third_rule->request->super->request_line->method == NO_METHOD);
  den_assert_str_eq(third_rule->request->super->headers->name, "Referer");
  den_assert_str_eq(third_rule->request->super->headers->value, "www.example.org");
}

void test_get_best_matching_rule(void) {
  struct rule_message_t *best_matching_rule;
  struct rule_message_t **matching_rules = calloc(4, sizeof(struct rule_message_t *));
  matching_rules[0] = &(struct rule_message_t) {
    .request=&(struct rule_request_t) {
      .accuracy = 10 }};
  matching_rules[1] = &(struct rule_message_t) {
    .request=&(struct rule_request_t) {
      .accuracy = 5 }};
  matching_rules[2] = &(struct rule_message_t) {
    .request=&(struct rule_request_t) {
      .accuracy = 20 }};
  matching_rules[3] = &(struct rule_message_t) {
    .request=&(struct rule_request_t) {
      .accuracy = 13 }};
  size_t matching_rules_length = 4;
  best_matching_rule = get_best_matching_rule(matching_rules, matching_rules_length);
  den_assert(best_matching_rule->request->accuracy == 20);
}

void server_test_suite(void) {
  RUN_TEST("test_add_to_matches", test_add_to_matches);
  RUN_TEST("test_collect_matching_rules_for_request", test_collect_matching_rules_for_request);
  RUN_TEST("test_get_best_matching_rule", test_get_best_matching_rule);
}
