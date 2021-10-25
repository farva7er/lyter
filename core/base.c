#include "base.h"
#include "host.h"
#include "log.h"
#include "../utils/string_utils.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

volatile sig_atomic_t is_running = 1;

void sigterm_handler(int code)
{
	is_running = 0;
}

int perform_fd_selection(int ls, fd_set *readfds, fd_set *writefds,
									struct connection * const connections);
void accept_connection(int ls, struct connection **connections);
void add_connection(int cs, struct connection **head);
void iterate_connections(fd_set *readfds, fd_set *writefds,
			struct connection **connections, struct host_node *hosts);

void add_new_request(struct connection *conn);
int fill_last_request(struct connection *conn);
char *get_crlf_line(struct connection *conn);
void read_remaining(struct connection *conn);
void flush_buffer(int fd, struct connection *conn);
void read_in_buffer(int fd, struct connection *conn);
void analyze_buffer(struct connection *conn);
void handle_requests(struct connection *conn, struct host_node *hosts);
void handle_ready_req(struct request *req, struct host_node *hosts);
int fill_response(struct request *req, struct response *resp,
												struct host_node *hosts);
static int get_handler(struct request *req, struct host_node *hosts,
				route_handler *out_handler, char **out_allowed_methods);

int handle_finished_req(struct connection *conn);
void remove_request(struct connection *conn);
void write_response(struct connection *conn);



int perform_fd_selection(int ls, fd_set *readfds, fd_set *writefds,
									struct connection *connections)
{
	int max_fd = ls;
	struct connection *curr_conn = connections;
	FD_ZERO(readfds);
	FD_SET(ls, readfds);
	FD_ZERO(writefds);
	
	while(curr_conn) {
		if(!curr_conn->closing && curr_conn->read_count < read_buff_length) {
			FD_SET(curr_conn->fd, readfds);
			max_fd = MAX(max_fd, curr_conn->fd);
		}
		if(curr_conn->write_count > 0) {
			FD_SET(curr_conn->fd, writefds);	
			max_fd = MAX(max_fd, curr_conn->fd);
		}
		curr_conn = curr_conn->next;
	}

	return max_fd;
}

void accept_connection(int ls, struct connection **connections)
{
	int cs, flags;
   struct sockaddr_in addr;
	socklen_t addrlen = sizeof(addr);
    cs = accept(ls, (struct sockaddr*) &addr, &addrlen);
    if(cs == -1) {
		write_log(LOG_ERR, ACCEPT_FAILED, NULL);
		return;
	}

	flags = fcntl(cs, F_GETFL);					/* in order to use writefds */
	fcntl(cs, F_SETFL, flags | O_NONBLOCK);

   add_connection(cs, connections);

	write_log(LOG_INFO, CONNECTION_CREATED, NULL);
}

void add_connection(int cs, struct connection **head)
{
	struct connection* conn = malloc(sizeof(*conn));
	conn->closing = 0;
	conn->fd = cs;
	conn->read_count = 0;
	conn->write_count = 0;
	conn->req_head = NULL;
	conn->req_tail = NULL;
	conn->next = *head;
	conn->prev = NULL;
	if(*head)
		(*head)->prev = conn;
	*head = conn;
}

void remove_connection(struct connection **head, struct connection *conn)
{
	if(conn->prev == NULL)
		*head = conn->next;
	else
		conn->prev->next = conn->next;
	if(conn->next)
		conn->next->prev = conn->prev;
	close(conn->fd);
	free_requests(conn->req_head);
	free(conn);
	
	write_log(LOG_INFO, CONNECTION_CLOSED, NULL);
}

int init_listening(int port)
{
   struct sockaddr_in addr;
   int res, opt = 1;

   /* listening socket */
   int ls = socket(AF_INET, SOCK_STREAM, 0);
   if(ls == -1) {
      perror("init_listening");
      return -1;
   }

   addr.sin_family = AF_INET;
   addr.sin_port = htons(port);
   addr.sin_addr.s_addr = htonl(INADDR_ANY);

   setsockopt(ls, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)); 

   res = bind(ls, (struct sockaddr*)&addr, sizeof(addr));
   if(res == -1) {
      perror("init_listening");
      return -1;
   }

   res = listen(ls, qlen);
   if(res == -1) {
      perror("init_listening");
      return -1;
   }

   /* drop root privileges */
   setuid(getuid());
   setgid(getgid());

   return ls;
}

int run(int ls, struct host_node *hosts)
{
	struct connection *connections = NULL;
	fd_set readfds;
	fd_set writefds;
	int max_fd;

	signal(SIGTERM, sigterm_handler);

	for(;;) {
		int res;
      struct timeval wake_up_time;

		max_fd = perform_fd_selection(ls, &readfds, &writefds, connections);

      wake_up_time.tv_sec = wake_up_time_sec;
      wake_up_time.tv_usec = 0;

      res = select(max_fd + 1, &readfds, &writefds, NULL, &wake_up_time);

      if(res == -1) {
			if(errno == EINTR) {
				write_log(LOG_INFO, SELECT_INTR, NULL);
				if(is_running == 0) {
					write_log(LOG_INFO, SHUTDOWN, "%%%%-----------------%%%%");
					return 0;
				} 
         } else {
				write_log(LOG_ERR, SELECT_ARG_ERR, NULL);
		   }
            continue;
      }

      if(res == 0) {
			write_log(LOG_DEBUG, SELECT_TIMEOUT, NULL);
            continue;
      }

 		if(FD_ISSET(ls, &readfds))
			accept_connection(ls, &connections);

		iterate_connections(&readfds, &writefds, &connections, hosts);
    }
}


void iterate_connections(fd_set *readfds, fd_set *writefds,
		struct connection **connections, struct host_node *hosts)
{
	struct connection *curr_con = *connections, *next;
	while(curr_con) {
		next = curr_con->next;
		if(FD_ISSET(curr_con->fd, writefds))
			flush_buffer(curr_con->fd, curr_con);
		if(FD_ISSET(curr_con->fd, readfds)) {
			read_in_buffer(curr_con->fd, curr_con);
			analyze_buffer(curr_con);
		}
		if(!curr_con->closing)
			handle_requests(curr_con, hosts);	
		else 
			remove_connection(connections, curr_con);
		curr_con = next;
	}

}

void flush_buffer(int fd, struct connection *conn)
{
	int wc = write(fd, conn->write_buff, conn->write_count);
	if(wc == -1) {
		write_log(LOG_ERR, WRITE_ERR, NULL);
		return;
	}
	memmove(conn->write_buff, conn->write_buff + wc, conn->write_count - wc);
	conn->write_count -= wc;
}

void read_in_buffer(int fd, struct connection *conn)
{
	int rc = read(fd, conn->read_buff + conn->read_count,
										read_buff_length - conn->read_count);
	if(rc == -1) {
		write_log(LOG_ERR, READ_ERR, NULL);
		conn->closing = 1;
		return;
	}
	if(rc == 0) {
		conn->closing = 1;
		return;
	}
	conn->read_count += rc;
}

void analyze_buffer(struct connection *conn)
{
	while(conn->read_count > 0) {
		int fill_success;
		if(!conn->req_tail || conn->req_tail->state != uncompleted)
			add_new_request(conn);

		fill_success = fill_last_request(conn);
		if(!fill_success)
			break;
	}
}

void add_new_request(struct connection *conn)
{
	struct request *req = malloc(sizeof(*req));
	req->req_line_set = 0;
	req->state = uncompleted;
	req->remains_to_read = 0;
	req->method = -1;
	req->path = NULL;
	req->http_version = NULL;
	req->headers_head = NULL;
	req->headers_tail = NULL;
	req->body = NULL;
	req->body_length = 0;
	req->resp = NULL;
	req->str_resp = NULL;
	req->resp_length = 0;
	req->next = NULL;
	if(!conn->req_tail) {
		conn->req_tail = req;
		conn->req_head = req;
	} else {
		conn->req_tail->next = req;
		conn->req_tail = req;
	}
}

int fill_last_request(struct connection *conn)
{
	char *crlf_line;
	struct request *req = conn->req_tail;
	if(req->remains_to_read > 0) {
		read_remaining(conn);
		if(req->remains_to_read == 0)
			req->state = ready;
		return 1;
	}
	crlf_line = get_crlf_line(conn);
	if(!crlf_line)								/* no crlf in buffer */
		return 0;
	if(crlf_line[0] == 0 && !req->req_line_set) {
		free(crlf_line);						/* skip empty line */
		return 1;
	}

	if(crlf_line[0] == 0) {						/* end of headers */
		int length = get_content_length(req);
		req->remains_to_read = length;
		if(length == 0) {
			req->state = ready;
		}
		free(crlf_line);
		return 1;
	}

	if(!req->path) {
		set_req_line_info(req, crlf_line);
		req->req_line_set = 1;
	} else {
		add_req_header_to_tail(req, make_header(crlf_line));
	}
	free(crlf_line);
	return 1;
}

void read_remaining(struct connection *conn)
{
	int rc, old_length, new_length;
	char *new_body;
	struct request *req = conn->req_tail;

	rc = MIN(req->remains_to_read, read_buff_length - conn->read_count);
	old_length = req->body_length;
	new_length = old_length + rc;
	new_body = malloc(new_length);
	memcpy(new_body, req->body, old_length);
	memcpy(new_body + old_length, conn->read_buff, rc);
	if(req->body)
		free(req->body);
	req->body = new_body;
	req->body_length = new_length;
	req->remains_to_read -= rc;

	memmove(conn->read_buff, conn->read_buff + rc,
						read_buff_length - conn->read_count);
	conn->read_count -= rc;
}

char *get_crlf_line(struct connection *conn)
{
	int i = 0, length = -1;
	char *line, *start = conn->read_buff;
	while(i + 1 < conn->read_count) {
		if(start[i] == '\r' &&
		   start[i + 1] == '\n') {
				length = i;
				break;
		}
		i++;
	}
	
	if(length == -1)
		return NULL;
	line = malloc(length + 1);
	strncpy(line, start, length);
	line[length] = 0;

	/* + 2 is added because of trailing CRLF */
	memmove(start, start + i + 2, conn->read_count - (length + 2));
	conn->read_count -= (length + 2);
	return line;
}


void handle_requests(struct connection *conn, struct host_node *hosts)
{
	struct request *curr_req = conn->req_head;
	int res;
	while(curr_req) {
		if(curr_req->state == ready)
			handle_ready_req(curr_req, hosts);
		curr_req = curr_req->next;
	}
	
	while(conn->req_head && conn->req_head->state == finished) {
		res = handle_finished_req(conn);
		if(-1 == res)
			break;
	}
}

void handle_ready_req(struct request *req, struct host_node *hosts)
{
	struct response *resp;
	int res;

	char * req_str = req_to_str(req);
	write_log(LOG_DEBUG, REQ_RECIEVED, req_str);
	free(req_str);
	
	resp = malloc(sizeof(*resp));
	resp->headers_head = NULL;
	resp->headers_tail = NULL;
	resp->body = NULL;
	resp->body_length = 0;
	res = fill_response(req, resp, hosts);
	req->state = res;
}

int fill_response(struct request *req,
						struct response *resp, struct host_node *hosts)
{
	int head_flag = 0;
	int res, state;
	char buff[256];
	const char not_found[] = "<h1>NOT FOUND :( </h1>";
	char *allowed_methods = NULL;
	route_handler handler;
	struct req_info req_info;

	req->resp = resp;
	resp->http_version = get_server_http_version();

	res = validate_http_version(req->http_version);
	if(-1 == res) {
		set_code_and_phrase(resp, HTTP_VERSION_NOT_SUPPORTED);
		sprintf(buff, 	"%s version is not supported by this server,"
						"please use %s version.",
						req->http_version, get_server_http_version());
		set_body(resp, buff, strlen(buff));
		state = finished;
		goto cleanup;
	}

	if(-1 == req->method) {
		set_code_and_phrase(resp, NOT_IMPLEMENTED);
		state = finished;
		goto cleanup;
	}

	if(req->method == http_head) {
		req->method = http_get;
		head_flag = 1;
	}

	res = get_handler(req, hosts, &handler, &allowed_methods);

	if(res == BAD_REQUEST) {
		const char msg[] = "Please specify valid host (host header)";
		set_code_and_phrase(resp, BAD_REQUEST);	
		set_body(resp, msg, sizeof(msg));
		state = finished;
		goto cleanup;
	}
	
	if(res == METHOD_NOT_ALLOWED) {
		if(allowed_methods && allowed_methods[0] == 0) {	
			set_code_and_phrase(resp, NOT_FOUND);
			set_body(resp, not_found, sizeof(not_found));
		} else {
			set_code_and_phrase(resp, METHOD_NOT_ALLOWED);
			add_header(resp, "Allowed", allowed_methods);
		}
		state = finished;
		goto cleanup;
	}

	if(res == NOT_FOUND) {	
		set_code_and_phrase(resp, NOT_FOUND);	
		set_body(resp, not_found, sizeof(not_found));
		state = finished;
		goto cleanup;
	}

	

	fill_req_info(&req_info, req);
	res = handler(&req_info, resp);

	


	if(res == NOT_FOUND)
		set_body(resp, not_found, sizeof(not_found));

	if(head_flag){
		if(resp->body)
			free(resp->body);
		resp->body = NULL;
		resp->body_length = 0;
		req->method = http_head;
	}

	if(res == 0) {
		state = processing;
	} else {
		set_code_and_phrase(resp, res);
		state = finished;
	}

cleanup:
	if(allowed_methods)
		free(allowed_methods);
	return state;
}


int get_handler(struct request *req, struct host_node *hosts,
						route_handler *out_handler, char **out_allowed_methods)
{
	p_route_node routes;
	int allowed_methods[http_method_count] = { 0 };
	const char *h = get_header_value(req->headers_head, "host");
	if(!h)
		return BAD_REQUEST;
	routes = search_host(hosts, h);
	if(!routes)
		return BAD_REQUEST;
	
	*out_handler = search_handler(routes, req->method, req->path,
															allowed_methods);
	if((route_handler) -1 == *out_handler)
		return NOT_FOUND;
	if((route_handler) 0 == *out_handler) {
		*out_allowed_methods = allowed_methods_str(allowed_methods);
		return METHOD_NOT_ALLOWED;
	}
	return 0;
}


int handle_finished_req(struct connection *conn)
{
	struct request *req = conn->req_head;
	write_response(conn);
	if(req->resp_length == 0) {
		remove_request(conn);
		return 0;
	}
	return -1;
}

void remove_request(struct connection *conn)
{
	struct request *req = conn->req_head;
	conn->req_head = conn->req_head->next;
	if(conn->req_head == NULL)
		conn->req_tail = NULL;
	free_request(req);
}

void write_response(struct connection *conn)
{
	int free_space, size;
	char *start;
	struct request *req = conn->req_head;
	if(!req->str_resp) {
		char buff[4];
		req->str_resp = response_to_str(req->resp, &req->resp_length);
		sprintf(buff, "%d", req->resp->code);
		write_log(LOG_DEBUG, RESP_WRITE, buff);
	}
	free_space = write_buff_length - conn->write_count;
	size = MIN(free_space, req->resp_length);
	start = conn->write_buff + conn->write_count;
	memcpy(start, req->str_resp, size);
	conn->write_count += size;
	memmove(req->str_resp, req->str_resp + size, req->resp_length - size);
	req->resp_length -= size;
}


