#include "set.h"

const char *root_path = "/home/farvater/site";

int index_handler(REQ *req, RESP *resp);
int file_handler(REQ *req, RESP *resp);


p_route_node set_routes()
{
	p_route_node root = NULL;
	add_route(&root, http_get, "/", index_handler);
	add_route(&root, http_get, "/*", file_handler);
	return root;
}

int index_handler(REQ *req, RESP *resp)
{
	int res = assign_file(resp, "/index.html");
	if(res == -1)
		return 404;
	return 200;

}

int file_handler(REQ *req, RESP *resp)
{
	int res = assign_file(resp, req->path);
	if(res == -1)
		return 404;
	return 200;
}


