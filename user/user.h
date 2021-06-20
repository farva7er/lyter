#ifndef USER_H_
#define USER_H_

#include "../core/http.h"
#include "../set.h"

char *get_file(const char *path, int *out_size);
int assign_file(struct response *resp, const char *file);
int redirect(struct response *resp, const char *path);

#endif

