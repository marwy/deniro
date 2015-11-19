#include <stdio.h>

#define RUN_TEST(function_name, test_fun, args...) test_fun(args); printf("Finished  %s\n", function_name);

void test_http_parse_method(char *buffer);
void test_http_parse_url(char *buffer);
void test_http_parse_version(char *buffer);
void test_http_parse_headers(char *buffer);
void test_http_parse_body(char *buffer);

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
}
