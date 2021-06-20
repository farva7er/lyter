#include "user.h"
#include "../core/http.h"
#include "../core/log.h"
#include "../utils/string_utils.h"
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>


const char *get_mime_type(const char *t);

int assign_file(struct response *resp, const char *path)
{
	struct token *file_tokens;
	char *file;
	int size;
	file = get_file(path, &size);
	if(file) {
		set_body(resp, file, size);
		file_tokens = tokenize(path, '.');
		if(file_tokens->next)
			add_header(resp, "Content-Type",
							 get_mime_type(file_tokens->next->value));
	} else {
		return -1;
	}
	free_tokens(file_tokens);
	return 0;
}

const char *get_mime_type(const char *t)
{
	if(0 == strcmp(t, "html"))
		return "text/html";
	else if(0 == strcmp(t, "css"))
		return "text/css";
	else if(0 == strcmp(t, "png"))
		return "image/png";
	else if(0 == strcmp(t, "jpeg"))
		return "image/jpeg";
	else if(0 == strcmp(t, "jpg"))
		return "image/jpeg";
	else if(0 == strcmp(t, "zip"))
		return "application/zip";
	else if(0 == strcmp(t, "tar"))
		return "application/gzip";
	return "text/plain";
}

char *get_file(const char *path, int *out_size)
{
	char *file, *abs_path;
	int rc, fd, seek;
	abs_path = str_add(root_path, path);
	fd = open(abs_path, O_RDONLY);
	if(fd == -1) {
		write_log(LOG_ERR, FILE_OPEN_ERR, abs_path);
		free(abs_path);
		return NULL;
	}
	seek = lseek(fd, 0, SEEK_END);
	if(seek == -1) {
		write_log(LOG_ERR, FILE_SEEK_ERR, abs_path);
		free(abs_path);
		return NULL;
	}
	*out_size = seek;
	file = malloc(*out_size);
	lseek(fd, 0, SEEK_SET);
	rc = read(fd, file, *out_size);
	if(rc == -1) {
		free(file);
		file = NULL;
		write_log(LOG_ERR, FILE_READ_ERR, abs_path);
	} 
	close(fd);
	free(abs_path);
	return file;
}

int redirect(struct response *resp, const char *path)
{
	add_header(resp, "Location", path);
	return 302;
}
