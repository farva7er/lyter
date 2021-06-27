#include "host.h"
#include <stdlib.h>
#include <string.h>

p_route_node *add_host(struct host_node **hosts, const char *name)
{
	char *n;
	struct host_node *new_node = malloc(sizeof(*new_node));
	int len = strlen(name);
	n = malloc(len + 1);
	strncpy(n, name, len + 1);
	new_node->name = n;
	new_node->routes = NULL;
	new_node->next = *hosts;
	*hosts = new_node;
	return &new_node->routes;
}

p_route_node search_host(const struct host_node *hosts, const char *name)
{
	while(hosts) {
		if(0 == strcmp(name, hosts->name))
			return hosts->routes;
		hosts = hosts->next;
	}
	return NULL;
}
