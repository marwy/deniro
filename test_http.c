#include "deniro_assert.h"
#include "http.h"


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

void test_get_last_http_header(struct http_header_t *headers_start) {
  struct http_header_t *last_header = get_last_http_header(headers_start);
  den_assert_str_eq(last_header->name, "Third header");
}

void test_get_last_http_header_with_only_one_header(struct http_header_t *headers_start) {
  struct http_header_t *last_header = get_last_http_header(headers_start);
  den_assert_str_eq(last_header->name, "Third header");
}

void test_copy_http_headers(void) {
  struct http_header_t *src_first = malloc(sizeof(struct http_header_t));
  src_first->name = "First header";
  src_first->value = "First header value";
  struct http_header_t *src_second = malloc(sizeof(struct http_header_t));
  src_second->name = "Second header";
  src_second->value = "Second header with different value";
  src_first->next_header = src_second;

  struct http_header_t *dest_first = malloc(sizeof(struct http_header_t));
  dest_first->name = "I'm an already existing header";
  dest_first->value = "I'm an already existing header's VALUE";
  struct http_header_t *dest_second = malloc(sizeof(struct http_header_t));
  dest_second->name = "Second header";
  dest_second->value = "Second header value";
  dest_first->next_header = dest_second;
  struct http_header_t *dest_third = malloc(sizeof(struct http_header_t));
  dest_third->name = "I'm an already existing header number 3";
  dest_third->value = "I'm an already existing header number 3's VALUE";
  dest_second->next_header = dest_third;

  copy_http_headers(&dest_first, src_first);
  den_assert_str_eq(dest_first->value,
                    "I'm an already existing header's VALUE");
  den_assert_str_eq(dest_first->next_header->value,
                    "Second header value");
  den_assert_str_eq(dest_first->next_header->next_header->value,
                    "I'm an already existing header number 3's VALUE");
  den_assert_str_eq(dest_first->next_header->next_header->next_header->value,
                    "First header value");
  // just making sure we have not accidentaly changed src's headers
  dest_first->next_header->next_header->next_header->value = "Test";
  den_assert_str_eq(src_first->value, "First header value");
}

void test_copy_http_request(void) {
  struct http_request_t *dest_request = http_request_new();
  dest_request->request_line->url = "/my-superduper-url/";
  dest_request->request_line->method = PUT;

  struct http_request_t *src_request = http_request_new();
  src_request->request_line->method = DELETE;
  src_request->body = "Imma delete you real hard.";
  struct http_header_t *headers_start = malloc(sizeof(struct http_header_t));
  headers_start->name = "First header";
  headers_start->value = "First header's value";
  src_request->headers = headers_start;

  copy_http_request(dest_request, src_request);
  den_assert_str_eq(dest_request->request_line->url, "/my-superduper-url/");
  den_assert(dest_request->request_line->method == PUT);
  den_assert_str_eq(dest_request->body, "Imma delete you real hard.");
  den_assert_str_eq(dest_request->headers->name, "First header");
  den_assert_str_eq(dest_request->headers->value, "First header's value");
}

void test_copy_http_response(void) {
  struct http_response_t *dest_response = http_response_new();
  dest_response->body = "You buchered something.";

  struct http_response_t *src_response = http_response_new();
  src_response->status_line->status_code = 500;
  src_response->status_line->reason_phrase = "Bad Request";
  src_response->body = "Something went wrong.";
  struct http_header_t *headers_start = malloc(sizeof(struct http_header_t));
  headers_start->name = "First header";
  headers_start->value = "First header's value";
  src_response->headers = headers_start;

  copy_http_response(dest_response, src_response);
  den_assert_str_eq(dest_response->body, "You buchered something.");
  den_assert(dest_response->status_line->status_code == 500);
  den_assert_str_eq(dest_response->status_line->reason_phrase,
                    "Bad Request");
  den_assert_str_eq(dest_response->headers->name, "First header");
  den_assert_str_eq(dest_response->headers->value, "First header's value");
}

void test_add_header(void) {
  struct http_header_t *headers = malloc(sizeof(struct http_header_t));
  headers->name = "First header";
  headers->value = "First header's value";

  struct http_header_t *second_header = malloc(sizeof(struct http_header_t));
  second_header->name = "Second header";
  second_header->value = "Second header's value";
  headers->next_header = second_header;

  struct http_header_t *third_header = malloc(sizeof(struct http_header_t));
  third_header->name = "Third header";
  third_header->value = "Third header's value";
  second_header->next_header = third_header;

  struct http_header_t *new_headers = add_header(headers, "Fourth header",
                                                 "Fourth header's value");
  den_assert_str_eq(new_headers->next_header->next_header->next_header->name,
                    "Fourth header");
  den_assert_str_eq(new_headers->next_header->next_header->next_header->value,
                    "Fourth header's value");
}

void test_add_header_with_same_name(void) {
  struct http_header_t *headers = malloc(sizeof(struct http_header_t));
  headers->name = "First header";
  headers->value = "First header's value";

  struct http_header_t *second_header = malloc(sizeof(struct http_header_t));
  second_header->name = "Second header";
  second_header->value = "Second header's value";
  headers->next_header = second_header;

  struct http_header_t *third_header = malloc(sizeof(struct http_header_t));
  third_header->name = "Third header";
  third_header->value = "Third header's value";
  second_header->next_header = third_header;

  struct http_header_t *new_headers = add_header(headers, "Second header",
                                                 "Second header's DIFFERENT value");
  den_assert(third_header->next_header == NULL); // no header was added
  // header's value was replaced
  den_assert_str_eq(new_headers->next_header->name,
                    "Second header");
  den_assert_str_eq(new_headers->next_header->value,
                    "Second header's DIFFERENT value");
}

void test_http_response_to_string(void) {
  struct http_response_t *response = http_response_new();
  response->headers = malloc(sizeof(struct http_header_t));
  response->headers->next_header = malloc(sizeof(struct http_header_t));

  response->status_line->http_version = "HTTP/1.1";
  response->status_line->status_code = 201;
  response->status_line->reason_phrase = "Created";
  response->body = "This thing you wanted to be made was indeed created.";
  response->headers->name = "Server";
  response->headers->value = "Taxi Driver";
  response->headers->next_header->name = "Content-Length";
  response->headers->next_header->value = "52";

  char *result = http_response_to_string(response);
  den_assert_str_eq(result,
                    "HTTP/1.1 201 Created\r\n"
                    "Server: Taxi Driver\r\n"
                    "Content-Length: 52\r\n"
                    "\r\n"
                    "This thing you wanted to be made was indeed created.");

}

void test_http_response_to_string_without_body(void) {
  struct http_response_t *response = http_response_new();
  response->headers = malloc(sizeof(struct http_header_t));
  response->headers->next_header = malloc(sizeof(struct http_header_t));

  response->status_line->http_version = "HTTP/1.1";
  response->status_line->status_code = 201;
  response->status_line->reason_phrase = "Created";
  response->headers->name = "Server";
  response->headers->value = "Taxi Driver";
  response->headers->next_header->name = "Content-Length";
  response->headers->next_header->value = "52";

  char *result = http_response_to_string(response);
  den_assert_str_eq(result,
                    "HTTP/1.1 201 Created\r\n"
                    "Server: Taxi Driver\r\n"
                    "Content-Length: 52\r\n"
                    "\r\n");

}

void test_http_response_to_string_without_content_length_header(void) {
  struct http_response_t *response = http_response_new();
  response->headers = malloc(sizeof(struct http_header_t));

  response->status_line->http_version = "HTTP/1.1";
  response->status_line->status_code = 201;
  response->status_line->reason_phrase = "Created";
  response->body = "This thing you wanted to be made was indeed created.";
  response->headers->name = "Server";
  response->headers->value = "Taxi Driver";

  char *result = http_response_to_string(response);
  den_assert_str_eq(result,
                    "HTTP/1.1 201 Created\r\n"
                    "Server: Taxi Driver\r\n"
                    "Content-Length: 52\r\n"
                    "\r\n"
                    "This thing you wanted to be made was indeed created.");

}
