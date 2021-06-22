#ifndef HTTP_H_
#define HTTP_H_

#include "http_const.h"
#include "routes.h"

struct header {
	char* name;
	char *value;
	struct header *next;
};

struct req_info;

enum req_states { uncompleted = 0, ready, processing, finished };


struct response {
	const char *http_version;
	int code;
	const char *phrase;
	struct header *headers_head;
	struct header *headers_tail;
	char *body;
	int body_length;
};


typedef struct response RESP;

struct request {
	int req_line_set;
    enum req_states state;
    int remains_to_read;
    enum http_method method;
    char *path;
	char *http_version;
    struct header *headers_head;
	struct header *headers_tail;
    char *body;
	int body_length;
	struct response *resp;
	char *str_resp;
	int resp_length;
    struct request *next;
};


int get_content_length(struct request *req);
void set_req_line_info(struct request *req, const char *request_line);
struct header *make_header(const char *line);
void add_req_header_to_tail(struct request *req, struct header *header);
void add_header(struct response *resp, const char *name, const char *value);
void free_request(struct request *req);
void free_requests(struct request *head);
enum http_method get_method(const char *m);
char *response_to_str(const struct response *resp, int *out_length);
void set_code_and_phrase(struct response *resp, int code);
int validate_http_version(const char *version);
const char *get_server_http_version();
void set_body(struct response *resp, const char *buff, int size);
char *allowed_methods_str(int *allowed_methods);
int calc_resp_length(const struct response *resp);
const char *get_phrase(int code);
char *req_to_str(const struct request *req);
#endif

