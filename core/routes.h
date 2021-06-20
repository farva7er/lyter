#ifndef ROUTES_H_
#define ROUTES_H_

#include "http.h"

#define  WILDCARD_CHAR '*'

struct req_info {
	enum http_method method;
	const char *path;
	const struct header *headers;
	const char *body;
	int body_length;
};

typedef struct req_info REQ;

struct request;
struct response;


typedef int (*route_handler)(struct req_info*, struct response*);

struct route_node {
	char *token;
	route_handler handlers[http_method_count];
	route_handler general_handlers[http_method_count];
	struct route_node *sub_nodes;
	struct route_node *next;
};

typedef struct route_node* p_route_node;

/* setting ///////////////////////////////////////////////////////// */
p_route_node set_routes();


void add_route(p_route_node *root, enum http_method method, const char *path,
													route_handler handler);
route_handler search_handler(p_route_node root, enum http_method method,
								const char *path, int *allowed_methods);
void fill_req_info(struct req_info *req_info, const struct request *req);
void free_routes(p_route_node head);

#endif
