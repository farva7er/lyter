#include "set.h"

const char *root_path = "/home/farvater/site";

int index_handler(REQ *req, RESP *resp);
int file_handler(REQ *req, RESP *resp);
int root2_handler(REQ *req, RESP *resp);

HOST set_hosts()
{
	HOST hosts = NULL;
	ROOT root1 = add_host(&hosts, "localhost:15000");
	ROOT root2 = add_host(&hosts, "127.0.0.1:15000");
	
	add_route(root1, http_get, "/", index_handler);
	add_route(root1, http_get, "/*", file_handler);


	add_route(root2, http_get, "/", index_handler);
	add_route(root2, http_get, "/*", root2_handler);
	return hosts;
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


int root2_handler(REQ *req, RESP *resp)
{
	set_body(resp, "ROOT 2", 6);
	return 200;
}

