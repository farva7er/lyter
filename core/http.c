#include "http.h"
#include "../utils/string_utils.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

enum { max_body_length_digits = 7 };

void free_headers(struct header *headers);
void free_header(struct header *header);
void free_response(struct response *resp);

const char *get_informational_phrase(int code);
const char *get_successful_phrase(int code);
const char *get_redirection_phrase(int code);
const char *get_client_error_phrase(int code);
const char *get_server_error_phrase(int code);


int get_content_length(struct request *req)
{
    struct header *curr_header = req->headers_head;
    while(curr_header) {
        if(0 == strcmp(curr_header->name, "content-length"))
            return atoi(curr_header->value);
        curr_header = curr_header->next;
    }
    return 0;
}


void set_req_line_info(struct request *req, const char *request_line)
{
    struct token* tokens;
    tokens = tokenize(request_line, ' ');
    if(tokens) {
        req->method = get_method(tokens->value);
        tokens->value = NULL;
		if(tokens->next) {
        	req->path = tokens->next->value;
        	tokens->next->value = NULL;
			if(tokens->next->next) {
        		req->http_version = tokens->next->next->value;
        		tokens->next->next->value = NULL;
    		}

    	}
    
    }
	free_tokens(tokens);
}

struct header *make_header(const char *line)
{
	char *name, *value;
	struct header *header;
	struct token *tokens = tokenize(line, ':');

	header = malloc(sizeof(*header));
	header->name = NULL;
	header->value = NULL;
	header->next = NULL;
	if(tokens) {
		name = trim(tokens->value);
		str_to_lower(name);
		header->name = name;
	}
	if(tokens->next) {
		value = trim(tokens->next->value);
		header->value = value;
	}
	free_tokens(tokens);
	return header;
}


void add_req_header_to_tail(struct request *req, struct header *header)
{
	if(req->headers_tail == NULL) {
		req->headers_head = header;
		req->headers_tail = header;
	} else {
		req->headers_tail->next = header;
		req->headers_tail = header;
	}
}

void add_req_header(struct request *req, const char *name, const char *value)
{
	struct header *h = malloc(sizeof(*h));
	h->name = copy_string(name);
	h->value = copy_string(value);
	h->next = NULL;
	add_req_header_to_tail(req, h);
}

void add_resp_header_to_tail(struct response *resp, struct header *header)
{
	if(resp->headers_tail == NULL) {
		resp->headers_head = header;
		resp->headers_tail = header;
	} else {
		resp->headers_tail->next = header;
		resp->headers_tail = header;
	}
}

void add_header(struct response *resp, const char *name, const char *value)
{
	struct header *h = malloc(sizeof(*h));
	h->name = copy_string(name);
	h->value = copy_string(value);
	h->next = NULL;
	add_resp_header_to_tail(resp, h);
}

void set_header(struct response *resp, const char *name, const char *value)
{
	int found = 0;
	struct header *h = resp->headers_head;
	while(h) {
		if(0 == strcmp_i(h->name, name)) {
			found = 1;
			break;
		}
		h = h->next;
	}

	if(found) {
		free(h->value);
		h->value = copy_string(value);
	} else {
		add_header(resp, name, value);
	}
}

char *response_to_str(const struct response *resp, int *out_length)
{
	char *str;
	int i = 0;
	struct header *curr_header = resp->headers_head;
	int length = calc_resp_length(resp);
	str = malloc(length + 1);
	i += sprintf(str + i, "%s %d %s\r\n", resp->http_version,
										resp->code, resp->phrase);
	while(curr_header) {
		i += sprintf(str + i, "%s: %s\r\n", curr_header->name,
												curr_header->value);
		curr_header = curr_header->next;
	}
	i += sprintf(str + i, "\r\n");
	if(resp->body_length)
		memcpy(str + i, resp->body, resp->body_length);
	*out_length = length;
	return str;
}

int calc_resp_length(const struct response *resp)
{
	int status_line_length, headers_length = 0, body_length;	
	struct header *curr_header = resp->headers_head;

	/* version SP code SP phrase CRLF */
	status_line_length = strlen(resp->http_version) + 1 + 3 + 1
							+ strlen(resp->phrase) + 2;
	headers_length = 0;
	while(curr_header) {
		/* name : SP value CRLF */
		headers_length += (strlen(curr_header->name) + 2
						+ strlen(curr_header->value) + 2);
		curr_header = curr_header->next;
	}
	headers_length += 2;	/* CRLF */
	body_length = resp->body_length;

	return (status_line_length + headers_length + body_length);
	
}

void free_request(struct request *req)
{
	if(req->path)
		free(req->path);
	if(req->http_version)
		free(req->http_version);
	free_headers(req->headers_head);
	if(req->body)
		free(req->body);
	if(req->resp)
		free_response(req->resp);
}

void free_response(struct response *resp)
{
	free_headers(resp->headers_head);
	if(resp->body)
		free(resp->body);
	
}

void free_requests(struct request *head)
{
	struct request *tmp;
	while(head) {
		tmp = head;
		head = head->next;
		free_request(tmp);
	}
}

void free_headers(struct header *headers)
{
	struct header *tmp;
	while(headers) {
		tmp = headers;
		headers = headers->next;
		free_header(tmp);
	}
}

void free_header(struct header *header)
{
	if(header->name)
		free(header->name);
	if(header->value)
		free(header->value);
	free(header);
}

enum http_method get_method(const char *m)
{
	if(0 == strcmp(m, "GET"))
		return http_get;
	else if(0 == strcmp(m, "POST"))
		return http_post;
	else if(0 == strcmp(m, "HEAD"))
		return http_head;
	return -1;
	/* TODO ADD ALL METHODS ------------------------------------- */
}

void set_code_and_phrase(struct response *resp, int code)
{
	resp->code = code;
	resp->phrase = get_phrase(code);
}

void set_body(struct response *resp, const char *body, int size)
{
	char buff[max_body_length_digits]; 

	if(resp->body)
		free(resp->body);
	if(size != 0)
		resp->body = malloc(size);
	else
		resp->body = NULL;
	resp->body_length = size;
	memcpy(resp->body, body, size);
	snprintf(buff, max_body_length_digits + 1, "%d", size);
	
	set_header(resp, "Content-Length", buff);
}

const char *get_server_http_version()
{
	return "HTTP/1.1";
}

int validate_http_version(const char *version)
{
	if(!version)
		return -1;
	if(0 == strcmp(version, get_server_http_version()))
		return 0;
	return -1;
}

const char *get_phrase(int code)
{
	int first_digit = code / 100;
	switch(first_digit) {
		case 1: 	return get_informational_phrase(code);
		case 2:		return get_successful_phrase(code);
		case 3: 	return get_redirection_phrase(code);
		case 4: 	return get_client_error_phrase(code);
		case 5: 	return get_server_error_phrase(code);
		default: 	return "Unknown Code";
	}
}

const char *get_informational_phrase(int code)
{
	switch(code) {
		case 100:	return "Continue";
		case 101:	return "Switching Protocols";
		default:	return "Unknown Code";
	}
}

const char *get_successful_phrase(int code)
{
	switch(code) {
		case 200:	return "OK";
		case 201:	return "Created";
		case 202: 	return "Accepted";
		case 203:	return "Non-Authoritative Information";
		case 204:	return "No Content";
		case 205:	return "Reset Content";
		case 206:	return "Partial Content";
		default:	return "Unknown Code";
	}
}

const char *get_redirection_phrase(int code)
{
	switch(code) {
		case 300:	return "Multiple Choices";
		case 301:	return "Moved Permanently";
		case 302:	return "Found";
		case 303:	return "See Other";
		case 304:	return "Not Modified";
		case 305:	return "Use Proxy";
		case 306:	return "(Unused)";
		case 307:	return "Temporary Redirect";
		default:	return "Unknown Code";
	}
}

const char *get_client_error_phrase(int code)
{
	switch(code) {
		case 400:	return "Bad Request";
		case 401:	return "Unauthorized";
		case 402:	return "Payment Required";
		case 403:	return "Forbidden";
		case 404:	return "Not Found";
		case 405:	return "Method Not Allowed";
		case 406:	return "Not Acceptable";
		case 407:	return "Proxy Authentication Required";
		case 408:	return "Request Timeout";
		case 409:	return "Conflict";
		case 410:	return "Gone";
		case 411:	return "Length Required";
		case 412:	return "Precondition Failed";
		case 413:	return "Request Entity Too Large";
		case 414:	return "Request-URI Too Long";
		case 415:	return "Unsupported Media Type";
		case 416:	return "Requested Range Not Satisfiable";
		case 417:	return "Expectation Failed";
		default:	return "Unknown Code";
	}
}

const char *get_server_error_phrase(int code)
{
	switch(code) {
		case 500:	return "Internal Server Error";
		case 501:	return "Not Implemented";
		case 502:	return "Bad Gateway";
		case 503:	return "Service Unavailable";
		case 504:	return "Gateway Timeout";
		case 505:	return "HTTP Version Not Supported";
		default:	return "Unknown Code";
	}
}

char *allowed_methods_str(int *allowed_methods)
{
	int i = 0;
	char *str = malloc(100); 	/* should be enough for all methods */
	str[0] = 0;
	if(allowed_methods[http_get])
		i += sprintf(str + i, "GET, ");
	if(allowed_methods[http_post])
		i += sprintf(str + i, "POST, ");
	if(allowed_methods[http_head])
		i += sprintf(str + i, "HEAD, ");
	if(i > 0)
		str[i - 2] = 0;			/* remove last ", " */
	return str;
	/* TODO ADD ALL HTTP METHODS  --------------------------------------- */
}

char *req_to_str(const struct request *req)
{
	struct header *h = req->headers_head;
	char *str = malloc(10000);
	int i = 0;
	i += sprintf(str, "state:%d r_t_r:%d\n"
				"method:%d %s %s", req->state, req->remains_to_read,
				req->method, req->path, req->http_version);
	while(h) {
		i += sprintf(str + i, "\n%s: %s", h->name, h->value);
		h = h->next;
	}
	return str;
}
