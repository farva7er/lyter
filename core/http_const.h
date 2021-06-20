#ifndef HTTPCONST_H_
#define HTTPCONST_H_


enum { http_method_count = 3 };

enum http_method { http_get = 0, http_post = 1, http_head = 2 };

enum {
	HTTP_VERSION_NOT_SUPPORTED = 505,
	NOT_IMPLEMENTED = 501,
	NOT_FOUND = 404,
	METHOD_NOT_ALLOWED = 405
};

#endif
