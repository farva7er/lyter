#ifndef STRING_UTILS_H_
#define STRING_UTILS_H_

struct token {
	char *value;
	struct token *next;
};

struct token *tokenize_first(const char *str, char separator);
struct token *tokenize(const char *str, char separator);
void free_tokens(struct token *head);

char *trim(const char *str);
char *get_str_lower(const char *str);
void str_to_lower(char *str);
void cut_begining(char *str, int count);
char *copy_string(const char *str);
char *get_last_token(const char *str, char separator);
char *str_add(const char *a, const char *b);
int strcmp_i(const char *a, const char *b);
#endif

