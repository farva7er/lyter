#include "string_utils.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

char *make_str(const char *begin, const char *end);

void add_token(struct token **head, struct token **tail, char *value);

int strcmp_i(const char *a, const char *b)
{
	for(;; a++, b++) {
		int d = tolower((unsigned char)*a) - tolower((unsigned char)*b);
		if(d || !*a)
			return d;
	}
}

char *str_add(const char *a, const char *b)
{
	char *str;
	int len1, len2;
	len1 = strlen(a);
	len2 = strlen(b);
	str = malloc(len1 + len2 + 1);
	strcpy(str, a);
	strcpy(str + len1, b);
	return str;	
}

struct token *tokenize(const char *str, char separator)
{
	struct token *head = NULL, *tail = NULL;
	const char *curr;
	/* str is indicating first symbol of token */
	if(*str == separator) 
		str++;
	curr = str;
	for(;;) {
		char *curr_str;
		if(*curr == 0 || *curr == separator) {
			curr_str = make_str(str, curr);
			if(curr_str)
				add_token(&head, &tail, curr_str);
			if(*curr == 0)
				break;
			str = curr + 1;						/* move to next token */
		}
		curr++;
	}
	return head;
}

char *get_last_token(const char *str, char separator)
{
	int len, i;
	len = strlen(str);
	i = len - 1;
	while(i >= 0 && str[i] != separator)
		i--;
	if(i == -1)
		return copy_string(str);
	return make_str(str + i + 1, str + len); 
}

char *copy_string(const char *str)
{
	char *copy;
	int len = strlen(str);
	copy = malloc(len + 1);
	strncpy(copy, str, len + 1);
	return copy;
}

/* [begin..end) */
char *make_str(const char *begin, const char *end)
{
	int length = end - begin;
	char *str;
	if(!length)
		return NULL;
	str = malloc(length + 1);
	strncpy(str, begin, length);
	str[length] = 0;
	return str;
}

void add_token(struct token **head, struct token **tail, char *value)
{
	struct token *new_token = malloc(sizeof(*new_token));
	new_token->value = value;
	new_token->next = NULL;
	if(*tail == NULL) {
		*head = new_token;
		*tail = *head;
	} else {
		(*tail)->next = new_token;
		*tail = new_token;
	}
}

void free_tokens(struct token *head)
{
	struct token *next;
	while(head) {
		next = head->next;
		if(head->value)
			free(head->value);
		free(head);
		head = next;
	}
}

char *trim(const char *str)
{
	int begin = 0, end = strlen(str) - 1;
	if(!str)
		return NULL;
	while(str[begin] && (str[begin] == ' ' || str[begin] == '\t'))
		begin++;
	while(end > begin && (str[end] == ' ' || str[end] == '\t'))
		end--;
	return make_str(str + begin, str + end + 1);	
}

char *get_str_lower(const char *str)
{
	char* new_str;
	int i = 0, len = strlen(str);
	new_str = malloc(len + 1);
	while(*str) {
		new_str[i] = tolower(*str);
		str++;
		i++;
	}
	new_str[len] = 0;
	return new_str;
}

void str_to_lower(char *str)
{
	while(*str) {
		*str = tolower(*str);
		str++;
	}
}

void cut_begining(char *str, int count)
{
	int length;
	if(!str)
		return;
	length = strlen(str);
	if(count >= length) {
		str[0] = 0;
		return;
	}
	memmove(str, str + count, length - count + 1);
}
