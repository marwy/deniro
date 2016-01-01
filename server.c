#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <stdbool.h>


#include <fcntl.h>
#include <sys/socket.h>
#include <sys/poll.h>
#include <netdb.h>

#include "http.h"
#include "rules_parser.h"


#define INITIAL_NUMBER_OF_DESCRIPTORS 2
#define RESIZE_DESCRIPTORS_AND_CONNECTIONS_BY_AMOUNT 100
#define MAX_MESSAGE_SIZE (sizeof(char) * 1024 * 8)

int NUMBER_OF_DESCRIPTORS = INITIAL_NUMBER_OF_DESCRIPTORS;

#define RESIZE_MATCHES_BY_AMOUNT 20

enum server_error {
  OUT_OF_ROOM = -1
};

struct connection {
  int socket_fd;
  char recv_buffer[MAX_MESSAGE_SIZE];
  char send_buffer[MAX_MESSAGE_SIZE];
};

struct connection *connections;
struct pollfd *socket_descriptors;

int resize_socket_descriptors_and_connections(int amount, int previous_number_of_descriptors,
                                               struct pollfd **socket_descriptors,
                                               struct connection **connections) {
  *socket_descriptors = realloc(*socket_descriptors, (NUMBER_OF_DESCRIPTORS + amount) * sizeof(struct pollfd));
  if (socket_descriptors != NULL) {
    NUMBER_OF_DESCRIPTORS = NUMBER_OF_DESCRIPTORS + amount;
    memset(&((*socket_descriptors)[previous_number_of_descriptors + 1]), 0, amount * sizeof(struct pollfd)); // this!
  }
  *connections = realloc(*connections, NUMBER_OF_DESCRIPTORS * sizeof(struct connection));
  if (connections != NULL) {
    memset(&(*connections)[previous_number_of_descriptors], 0, amount * sizeof(struct connection));
    return 0;
  }
  fprintf(stderr, "resizing socket descriptors and connections failed\n");
  exit(EXIT_FAILURE);
}


int add_new_client(struct pollfd socket_descriptors[], int socket) {
  struct pollfd *sd = socket_descriptors;
  for(int i = 0; i < NUMBER_OF_DESCRIPTORS; i++) {
    if (sd[i].fd == 0) {
      sd[i].fd = socket;
      sd[i].events = POLLIN | POLLOUT;
      return 0;
    }
  }
  return OUT_OF_ROOM;
}

int on_accept_new_client(struct pollfd **sd, size_t sd_index, struct addrinfo *addrinfo_result, struct connection **connections) {
  struct pollfd *socket_descriptors = *sd;
  int new_client = accept(socket_descriptors[sd_index].fd, addrinfo_result->ai_addr,
                          &addrinfo_result->ai_addrlen);
  if (new_client == -1) {
    perror("accepting new client failed");
    return -1;
  }
  else {
    int add_new_client_result = add_new_client(socket_descriptors, new_client);
    if (add_new_client_result == OUT_OF_ROOM) {
      int old_number_of_descriptors = NUMBER_OF_DESCRIPTORS;
      int resize_sd_and_conns_result = resize_socket_descriptors_and_connections(RESIZE_DESCRIPTORS_AND_CONNECTIONS_BY_AMOUNT,
                                                                                 old_number_of_descriptors,
                                                                                 sd, connections);
      if (resize_sd_and_conns_result != 0) {
        fprintf(stderr, "Resizing borked but didn't crash, how is that even possible?\n");
        abort();
      }
      int add_new_client_after_oor_result = add_new_client(*sd, new_client);
      if (add_new_client_after_oor_result != 0) {
        fprintf(stderr, "adding new client resulted in OUT_OF_ROOM twice in on go. Cosmic rays?\n");
        return -1;
      }
    }
  }
  return 0;
}

int on_receive_from_client(struct pollfd socket_descriptors[], size_t sd_index) {
  char message[MAX_MESSAGE_SIZE];
  ssize_t bytes_received = 0;
  bytes_received = recv(socket_descriptors[sd_index].fd, message, MAX_MESSAGE_SIZE, 0);
  if (bytes_received == -1) {
    perror("recv");
    return -1;
  }
  else if (bytes_received == 0) {
    // client closed the connection
    memset(&socket_descriptors[sd_index], 0, sizeof(struct pollfd));
  }
  else {
    connections[sd_index] = (struct connection) {.socket_fd=socket_descriptors[sd_index].fd};
    strcpy(connections[sd_index].recv_buffer, message);
    memset(message, 0, MAX_MESSAGE_SIZE);
  }
  return 0;
}

int on_send_to_client(size_t sd_index) {
  struct connection conn = connections[sd_index];
  size_t total_sent = 0;
  int bytes_sent = 0;
  size_t buffer_actual_length = strlen(conn.send_buffer);
  bytes_sent = send(conn.socket_fd, conn.send_buffer, buffer_actual_length, 0);
  if (bytes_sent != -1) {
    while ((total_sent += bytes_sent)  && (total_sent < buffer_actual_length)) {
      bytes_sent = send(conn.socket_fd, &(conn.send_buffer[total_sent]), buffer_actual_length-total_sent, 0);
      if (bytes_sent != -1) {
      }
      else {
        perror("resend");
        return -1;
      }
    }
  }
  else {
    perror("send");
    return -1;
  }

  connections[sd_index] = (struct connection ) {0};
  total_sent = 0;
  bytes_sent = 0;
  return 0;
}

struct rule_message_t **add_to_matches(struct rule_message_t *matches[],
                                      ssize_t *matches_reserved_length,
                                      struct rule_message_t *message,
                                      ssize_t *index_last_written_to) {
  *index_last_written_to = *index_last_written_to + 1;
  if (*index_last_written_to + 1 > *matches_reserved_length) {
    struct rule_message_t **temp;
    temp = realloc(matches, sizeof(struct rule_message_t *) * (*matches_reserved_length + RESIZE_MATCHES_BY_AMOUNT));
    if (temp == NULL) {
      fprintf(stderr, "Couldn't reallocate matches\n");
      exit(EXIT_FAILURE);
    }
    *matches_reserved_length = *matches_reserved_length + RESIZE_MATCHES_BY_AMOUNT;
    matches = temp;
  }
  matches[*index_last_written_to] = message;
  return matches;
};

struct rule_message_t **collect_matching_rules_for_request(struct http_request_t *parsed_request,
                                                          struct rule_message_t *rule_messages,
                                                          size_t *matching_rules_length) {
  ssize_t matches_reserved_capacity = 10;
  struct rule_message_t **matches = calloc(matches_reserved_capacity, sizeof(struct rule_message_t *));
  ssize_t index_last_written_to = -1;

  struct rule_message_t *temp_message = rule_messages;
  while (temp_message) {
    struct http_request_t *request = temp_message->request->super;
    if (request->request_line->url) {
      if (!(strcmp(request->request_line->url, parsed_request->request_line->url) == 0))
        goto no_match_continue;
    }
    if (request->request_line->method) {
      if (!(request->request_line->method == parsed_request->request_line->method))
        goto no_match_continue;
    }
    if (request->body) {
      if (!(strcmp(request->body, parsed_request->body) == 0))
        goto no_match_continue;
    }
    if (request->headers) {
      struct http_header_t *rule_header = request->headers;
      while (rule_header) {
        struct http_header_t *parsed_request_header = parsed_request->headers;
        bool found = false;
        while (parsed_request_header) {
          if (strcmp(rule_header->name, parsed_request_header->name) == 0) {
            found = true;
            if (!(strcmp(rule_header->value, parsed_request_header->value) == 0)) {
              goto no_match_continue;
            }
          }
          parsed_request_header = parsed_request_header->next_header;
        }
        if (!found)
          goto no_match_continue;
        rule_header = rule_header->next_header;
      }
    }
    matches = add_to_matches(matches, &matches_reserved_capacity, temp_message, &index_last_written_to);
  no_match_continue:
    temp_message = temp_message->next;
  }

  *matching_rules_length = index_last_written_to + 1;
  return matches;
}

struct rule_message_t *get_best_matching_rule(struct rule_message_t *matching_rules[],
                                              size_t matching_rules_length) {
  struct rule_message_t *best_match = matching_rules[0];
  for(size_t i = 0; i < matching_rules_length; i++) {
    if (matching_rules[i]->request->accuracy > best_match->request->accuracy)
      best_match = matching_rules[i];
  }
  return best_match;
}

int server_loop(struct rule_message_t *rule_messages) {

  struct addrinfo *res, hints;
  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;

  socket_descriptors = malloc(INITIAL_NUMBER_OF_DESCRIPTORS * sizeof(struct pollfd));
  memset(socket_descriptors, 0, INITIAL_NUMBER_OF_DESCRIPTORS * sizeof(struct pollfd));

  connections = malloc((INITIAL_NUMBER_OF_DESCRIPTORS + 1) * sizeof(struct connection));
  memset(connections, 0, (INITIAL_NUMBER_OF_DESCRIPTORS + 1) * sizeof(struct connection));

  if (getaddrinfo(NULL, "2000", &hints, &res) == -1) {
    perror("getaddrinfo");
  }

  int master_socket = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
  if (master_socket == -1) {
    perror("main socket initialization");
  }

  int yes = 1;
  if (setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) != 0) {
    perror("setsockopt");
  }

  if (bind(master_socket, res->ai_addr, res->ai_addrlen) != 0) {
    perror("bind");
  }

  listen(master_socket, 128); // backlog is 128 on Linux anyway

  socket_descriptors[0] = (struct pollfd) { .fd=master_socket, .events=POLLIN | POLLOUT};

  if (fcntl(master_socket, F_SETFL, O_NONBLOCK) == -1) {
    perror("fctnl nonblock");
  }

  while(1) {
    int poll_result = poll(socket_descriptors, NUMBER_OF_DESCRIPTORS + 1, -1);
    if (poll_result == -1) {
      perror("poll");
    }
    else {
      for(int i = 0; i < NUMBER_OF_DESCRIPTORS; i++) {
        if (socket_descriptors[i].revents & POLLIN) {
          if (socket_descriptors[i].fd == master_socket) {
            int accepted = on_accept_new_client(&socket_descriptors, i, res, &connections);
            if (accepted != 0) {
              fprintf(stderr, "Adding new client descriptor failed\n");
            }
          }
          else {
            int received = on_receive_from_client(socket_descriptors, i);
            if (received == -1)
              fprintf(stderr, "on_receive_from_client failed\n");
            else {
              // we don't care about empty buffers
              if (!strlen(connections[i].recv_buffer))
                continue;
              struct http_request_t *parsed_request = parse_http_request(connections[i].recv_buffer);
              char *response_string;

              struct rule_message_t **matching_rules, *best_matching_rule;
              size_t matching_rules_length = 0;
              matching_rules = collect_matching_rules_for_request(parsed_request,
                                                                  rule_messages,
                                                                  &matching_rules_length);
              if (matching_rules_length != 0) {
                best_matching_rule = get_best_matching_rule(matching_rules,
                                                            matching_rules_length);
                response_string = http_response_to_string(best_matching_rule->response->super);
              } else {
                printf("**** NO matching response for url: %s\n", parsed_request->request_line->url);
                // maybe change it to 400?
                struct http_response_t *response = http_response_new();
                response->status_line->status_code = 404;
                response->status_line->reason_phrase = "Not Found";
                response->body = "Couldn't match any rule to this request.";
                response_string = http_response_to_string(response);
              }
              memcpy(connections[i].send_buffer, response_string, strlen(response_string));
            }
          }
        }
        else if (socket_descriptors[i].revents & POLLOUT) {
          if (socket_descriptors[i].fd != master_socket) {
              struct connection conn = connections[i];
              if ((conn.socket_fd != 0) && (conn.socket_fd == socket_descriptors[i].fd)) {
                int sent = on_send_to_client(i);
                if (sent == -1)
                  fprintf(stderr, "on_send_to_client failed\n");
              }
          }
        }
        else if (socket_descriptors[i].revents & POLLHUP) {
          printf("client disconnected (POLLHUP), or something\n");
        }
        else if (socket_descriptors[i].revents & POLLERR) {
          perror("socket POLLERR");
        }
      }
    }
  }

  freeaddrinfo(res);
  free(socket_descriptors);
  free(connections);
}
