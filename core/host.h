#ifndef HOST_H_
#define HOST_H_

#include "routes.h"

struct host_node {
	const char *name;
	p_route_node routes;
	struct host_node *next;
};

typedef struct host_node *HOST;

p_route_node * add_host(struct host_node **hosts, const char *name);
p_route_node search_host(const struct host_node *hosts, const char *name);

#endif
