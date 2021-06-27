#include "log.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

static FILE *log_file;
const char *log_filename = "lyter.log";
static int max_level;

static struct log_msg messages[] = {
	{ SERVER_STARTED, "Server has been started" },
	{ SHUTDOWN, "Server is shutting down" },
	{ CONNECTION_CREATED, "A Connection was created" },
	{ ACCEPT_FAILED, "Client socket could not be accepted" },
	{ SELECT_INTR, "The call for select was interrupted" },
	{ SELECT_ARG_ERR, "Arguments for select were set incorrectly" },
	{ SELECT_TIMEOUT, "Timeout for select" },
	{ WRITE_ERR, "Couldn't write(flush) buffer for a connection" },
	{ READ_ERR, "Couldn't read in buffer for a connection" },
	{ CONNECTION_CLOSED, "Connection closed" },
	{ FILE_OPEN_ERR, "Couldn't open file" },
	{ FILE_READ_ERR, "Couldn't read from file" },
	{ FILE_SEEK_ERR, "Unsuccessful lseek for file" },
	{ REQ_RECIEVED, "Request recieved" },
	{ RESP_WRITE, "Response has been made" }
};


const char *find_msg(int msg_code);

const char *find_msg(int msg_code)
{
	int i;
	for(i = 0; i < sizeof(messages); i++) {
		if(messages[i].msg_code  == msg_code) {
			return messages[i].msg;
		}
	}
	return NULL;
}

int log_init(int m_level)
{
	log_file = fopen(log_filename, "a");
	if(!log_file) {
		perror(log_filename);
		return -1;
	}
	max_level = m_level;
	write_log(LOG_INFO, SERVER_STARTED, "!!----------------!!");
	return 0;	
}

void log_close()
{
	if(log_file)
		fclose(log_file);
}

void write_log(int level, int msg_code, const char *data)
{
	const char *level_msg, *time_msg, *msg;
	time_t log_time;
	if(level > max_level)
		return;
	switch(level) {
		case LOG_INFO:
			level_msg = "INFO: ";
			break;
		case LOG_ERR:
			level_msg = "ERROR: ";
			break;
		default:
			level_msg = "OTHER: ";
	}
	time(&log_time);
	time_msg = ctime(&log_time);
	if(time_msg)
		fprintf(log_file, time_msg);
	fprintf(log_file, level_msg);
	msg = find_msg(msg_code);
	if(msg) {
		fprintf(log_file, msg);
		fputc(' ', log_file);
	}
	if(data)
		fprintf(log_file, data);
	fputc('\n', log_file);	
	fputc('\n', log_file);
	fflush(log_file);
}

