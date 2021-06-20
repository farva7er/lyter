#include "routes.h"
#include "../utils/string_utils.h"
#include <stdlib.h>
#include <string.h>

enum { token_max = 100 };


p_route_node create_route_node(char *token);
p_route_node search_child(p_route_node node, const char *token);
void add_child(p_route_node node, const char *token, p_route_node* out_child);
void set_handler(p_route_node node, enum http_method method,
												route_handler handler);
void set_general_handler(p_route_node node, enum http_method method,
												route_handler handler);

static route_handler get_handler(p_route_node node, enum http_method method);

route_handler get_general_handler(p_route_node node, enum http_method method);

void handler_array_or(int *dest, const route_handler *source);


void add_route(p_route_node *root, enum http_method method,
							const char *path, route_handler handler)
{
	struct token *curr_token, *all_tokens;
	p_route_node curr_route_node;
	if(*root == NULL)
		*root = create_route_node(NULL);
	curr_route_node = *root;
	all_tokens = tokenize(path, '/');
	curr_token = all_tokens;
	for(;;) {
		p_route_node child;
		if(curr_token == NULL) {
			set_handler(curr_route_node, method, handler);
			break;
		}

		/* SPECIAL CASE  */
		if(curr_token->value[0] == WILDCARD_CHAR) {
			set_general_handler(curr_route_node, method, handler);
			break;
		}

		child = search_child(curr_route_node, curr_token->value);
		if(!child)
			add_child(curr_route_node, curr_token->value, &child);
		curr_route_node = child;
		curr_token = curr_token->next;	
	}
	free_tokens(all_tokens);
}

route_handler search_handler(p_route_node root, enum http_method method,
							const char *path, int *out_allowed_methods)
{
	struct token *curr_token, *all_tokens;
	p_route_node curr_route_node = root;
	route_handler handler;
	route_handler general_handler = get_general_handler(root, method);

	if(!general_handler)
		general_handler = (route_handler) -1;
	all_tokens = tokenize(path, '/');
	curr_token = all_tokens;

	if(all_tokens && out_allowed_methods)
		handler_array_or(out_allowed_methods, root->general_handlers);

	if(!all_tokens) {
		if(out_allowed_methods)
			handler_array_or(out_allowed_methods, root->handlers);
		return get_handler(root, method);
	}

	while(curr_token) {
		route_handler child_gen_handler;
		p_route_node child = search_child(curr_route_node, curr_token->value);
		if(!child) {
			free_tokens(all_tokens);
			return general_handler;
		}
		if(out_allowed_methods && curr_token->next)
			handler_array_or(out_allowed_methods, child->general_handlers);
		
		child_gen_handler = get_general_handler(child, method);
		if(child_gen_handler && curr_token->next)
			general_handler = child_gen_handler;
		curr_route_node = child;
		curr_token = curr_token->next;	
	}
	free_tokens(all_tokens);
	if(out_allowed_methods)
		handler_array_or(out_allowed_methods, curr_route_node->handlers);
	handler = get_handler(curr_route_node, method);
	if(handler)
		return handler;
	else if(general_handler == (route_handler) -1)
		return handler;
	else 
		return general_handler;
}

void handler_array_or(int *dest, const route_handler *source)
{
	int i;
	for(i = 0; i < http_method_count; i++)
		if(source[i])
			dest[i] = 1;
}

route_handler get_handler(p_route_node node, enum http_method method)
{
	return node->handlers[method];
}

route_handler get_general_handler(p_route_node node, enum http_method method)
{
	return node->general_handlers[method];
}

p_route_node search_child(p_route_node node, const char *token)
{
	p_route_node sub_node = node->sub_nodes;
	while(sub_node) {
		if(strcmp(sub_node->token, token) == 0)
			return sub_node;
		sub_node = sub_node->next;
	}
	return NULL;
}

void add_child(p_route_node node, const char *token, p_route_node *out_child)
{
	int len;
	p_route_node child = malloc(sizeof(*child));
	len = strlen(token);
	child->token = malloc(len + 1);
	strncpy(child->token, token, len + 1);
	memset(child->handlers, 0, sizeof(child->handlers));
	memset(child->general_handlers, 0, sizeof(child->general_handlers));
	child->sub_nodes = NULL;
	child->next = node->sub_nodes;
	node->sub_nodes = child;
	*out_child = child;
}

void set_handler(p_route_node node, enum http_method method,
										route_handler handler)
{
	node->handlers[method] = handler;
}

void set_general_handler(p_route_node node, enum http_method method,
												route_handler handler)
{
	node->general_handlers[method] = handler;
}


void free_routes(p_route_node head)
{
	p_route_node next;
	if(!head)
		return;
	next = head->next;
	free_routes(head->sub_nodes);
	if(head->token)
		free(head->token);
	free(head);
	free_routes(next);
}


p_route_node create_route_node(char *token)
{
	p_route_node node = malloc(sizeof(*node));
	node->token = token;
	memset(node->handlers, 0, sizeof(node->handlers));
	memset(node->general_handlers, 0, sizeof(node->general_handlers));
	node->sub_nodes = NULL;
	node->next = NULL;
	return node;
}

void fill_req_info(struct req_info *req_info, const struct request *req)
{
    req_info->method = req->method;
    req_info->path = req->path;
    req_info->headers = req->headers_head;
    req_info->body = req->body;
	req_info->body_length = req->body_length;
}

