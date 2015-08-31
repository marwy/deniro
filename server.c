#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>


#include <fcntl.h>
#include <sys/socket.h>
#include <sys/poll.h>
#include <netdb.h>


#define INITIAL_NUMBER_OF_DESCRIPTORS 2
#define RESIZE_BY_AMOUNT 100
#define MAX_MESSAGE_SIZE (sizeof(char) * 1024 * 8)

int NUMBER_OF_DESCRIPTORS = INITIAL_NUMBER_OF_DESCRIPTORS;

enum Error {
  OUT_OF_ROOM = -1
};

struct Connection {
  int socket_fd;
  char recv_buffer[MAX_MESSAGE_SIZE];
  char send_buffer[MAX_MESSAGE_SIZE];
};

struct Connection *connections;
struct pollfd *socket_descriptors;

int resize_socket_descriptors_and_connections(int amount, int previous_number_of_descriptors,
                                               struct pollfd **socket_descriptors,
                                               struct Connection **connections) {
  *socket_descriptors = realloc(*socket_descriptors, (NUMBER_OF_DESCRIPTORS + amount) * sizeof(struct pollfd));
  if (socket_descriptors != NULL) {
    NUMBER_OF_DESCRIPTORS = NUMBER_OF_DESCRIPTORS + amount;
    memset(&((*socket_descriptors)[previous_number_of_descriptors + 1]), 0, amount * sizeof(struct pollfd)); // this!
  }
  *connections = realloc(*connections, NUMBER_OF_DESCRIPTORS * sizeof(struct Connection));
  if (connections != NULL) {
    memset(&(*connections)[previous_number_of_descriptors], 0, amount * sizeof(struct Connection));
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

int on_accept_new_client(struct pollfd **sd, size_t sd_index, struct addrinfo *addrinfo_result, struct Connection **connections) {
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
      int resize_sd_and_conns_result = resize_socket_descriptors_and_connections(RESIZE_BY_AMOUNT, old_number_of_descriptors,
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

int on_receive_from_client(struct pollfd socket_descriptors[], size_t sd_index, char message[]) {
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
    connections[sd_index] = (struct Connection) {.socket_fd=socket_descriptors[sd_index].fd};
    strcpy(connections[sd_index].recv_buffer, message);
    strcpy(connections[sd_index].send_buffer, "HTTP/1.1 200 OK\r\nConnection: keep-alive\r\n\r\n");
    memset(message, 0, MAX_MESSAGE_SIZE);
  }
  return 0;
}

int on_send_to_client(struct pollfd socket_descriptors[], size_t sd_index) {
  struct Connection conn = connections[sd_index];
  int total_sent = 0;
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

  connections[sd_index] = (struct Connection ) {0};
  total_sent = 0;
  bytes_sent = 0;
  return 0;
}


int server_loop() {

  struct addrinfo *res, hints;
  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;

  socket_descriptors = malloc(INITIAL_NUMBER_OF_DESCRIPTORS * sizeof(struct pollfd));
  memset(socket_descriptors, 0, INITIAL_NUMBER_OF_DESCRIPTORS * sizeof(struct pollfd));

  connections = malloc((INITIAL_NUMBER_OF_DESCRIPTORS + 1) * sizeof(struct Connection));
  memset(connections, 0, (INITIAL_NUMBER_OF_DESCRIPTORS + 1) * sizeof(struct Connection));

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

  char message[MAX_MESSAGE_SIZE];

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
            int received = on_receive_from_client(socket_descriptors, i, message);
            if (received == -1)
              fprintf(stderr, "on_receive_from_client failed\n");
          }
        }
        else if (socket_descriptors[i].revents & POLLOUT) {
          if (socket_descriptors[i].fd != master_socket) {
              struct Connection conn = connections[i];
              if ((conn.socket_fd != 0) && (conn.socket_fd == socket_descriptors[i].fd)) {
                int sent = on_send_to_client(socket_descriptors, i);
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
