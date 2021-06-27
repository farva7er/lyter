#ifndef BASE_H_
#define BASE_H_

#include "host.h"
#include "http.h"

enum { wake_up_time_sec = 30, qlen = 5 };
enum { read_buff_length = 4096, write_buff_length = 4096 };

struct connection {
	int closing;
	int fd;
	char read_buff[read_buff_length];
	int read_count;
	char write_buff[write_buff_length];
	int write_count;
	struct request *req_head;
	struct request *req_tail;
	struct connection *next;
	struct connection *prev;
};


int init_listening(int port);
int run(int ls, struct host_node *hosts);

#endif
