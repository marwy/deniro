#ifndef SERVER_H
#define SERVER_H

struct rule_message_t;
int server_loop(struct rule_message_t *rule_messages, const char *server_port);

#endif
