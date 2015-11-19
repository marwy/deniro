#include "deniro_assert.h"
#include "http.h"

struct http_request_t *parse_http_request(char *buffer);


void test_http_parse_method(char *buffer) {
  struct http_request_t *request = parse_http_request(buffer);
  den_assert(request);
  den_assert(request->request_line->method == POST);
}

void test_http_parse_url(char *buffer) {
  struct http_request_t *request = parse_http_request(buffer);
  den_assert(request);
  den_assert_str_eq(request->request_line->url, "/test-url/");
}

void test_http_parse_version(char *buffer) {
  struct http_request_t *request = parse_http_request(buffer);
  den_assert(request);
  den_assert_str_eq(request->request_line->http_version, "HTTP/1.1");
}

void test_http_parse_headers(char *buffer) {
  struct http_request_t *request = parse_http_request(buffer);
  den_assert(request);
  den_assert_str_eq(request->headers->name, "Host");
  den_assert_str_eq(request->headers->value, "localhost");
  den_assert_str_eq(request->headers->next_header->name, "User-Agent");
  den_assert_str_eq(request->headers->next_header->value, "I'm a robot 0.1");
  den_assert_str_eq(request->headers->next_header->next_header->name, "Content-Length");
  den_assert_str_eq(request->headers->next_header->next_header->value, "23");
}

void test_http_parse_body(char *buffer) {
  struct http_request_t *request = parse_http_request(buffer);
  den_assert(request);
  den_assert_str_eq(request->body, "delete=this&update=that");
}
