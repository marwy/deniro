#include <stdio.h>
#include <stdlib.h>

#include "http.h"
#include "deniro_assert.h"

void test_http_parse_method(char *buffer);
void test_http_parse_url(char *buffer);
void test_http_parse_version(char *buffer);
void test_http_parse_headers(char *buffer);
void test_http_parse_body(char *buffer);
void test_get_last_http_header(struct http_header_t *headers_start);
void test_get_last_http_header_with_only_one_header(struct http_header_t *headers_start);
void test_copy_http_headers(void);
void test_copy_http_request(void);
void test_copy_http_response(void);
void test_add_header(void);
void test_add_header_with_same_name(void);

void test_add_to_matches(void);
void test_collect_matching_rules_for_request(void);

void rules_parser_test_suite(void);

int main() {
  char *buffer = "POST /test-url/ HTTP/1.1\r\nHost: localhost\r\nUser-Agent: I'm a robot 0.1\r\nContent-Length: 23\r\n\r\ndelete=this&update=that";

  RUN_TEST("test_http_parse_method", test_http_parse_method, buffer);
  RUN_TEST("test_http_parse_url", test_http_parse_url, buffer);
  RUN_TEST("test_http_parse_version", test_http_parse_version, buffer);
  RUN_TEST("test_http_parse_headers", test_http_parse_headers, buffer);
  RUN_TEST("test_http_parse_body", test_http_parse_body, buffer);

  char *buffer_without_body = "POST /test-url/ HTTP/1.1\r\nHost: localhost\r\nUser-Agent: I'm a robot 0.1\r\nContent-Length: 23\r\n\r\n";
  RUN_TEST("test_http_parse_method_without_body", test_http_parse_method, buffer_without_body);
  RUN_TEST("test_http_parse_url_without_body", test_http_parse_url, buffer_without_body);
  RUN_TEST("test_http_parse_version_without_body", test_http_parse_version, buffer_without_body);
  RUN_TEST("test_http_parse_headers_without_body", test_http_parse_headers, buffer_without_body);


  struct http_header_t *headers_start = malloc(sizeof(struct http_header_t));
  headers_start->name = "First header";
  headers_start->value = "First header value";
  struct http_header_t *second_header = malloc(sizeof(struct http_header_t));
  second_header->name = "Second header";
  second_header->value = "Second header value";
  headers_start->next_header = second_header;
  struct http_header_t *third_header = malloc(sizeof(struct http_header_t));
  third_header->name = "Third header";
  third_header->value = "Third header value";
  second_header->next_header = third_header;
  RUN_TEST("test_get_last_http_header", test_get_last_http_header, headers_start);
  RUN_TEST("test_get_last_http_header_with_only_one_header", test_get_last_http_header_with_only_one_header, headers_start);

  RUN_TEST("test_copy_http_headers", test_copy_http_headers);
  RUN_TEST("test_copy_http_request", test_copy_http_request);
  RUN_TEST("test_copy_http_response", test_copy_http_response);

  RUN_TEST("test_add_header", test_add_header);
  RUN_TEST("test_add_header_with_same_name", test_add_header_with_same_name);

  rules_parser_test_suite();
}
