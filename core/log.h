#ifndef LOG_H_
#define LOG_H_


enum levels { LOG_ERR = 0, LOG_INFO = 1, LOG_DEBUG = 2 };
enum codes {
	SERVER_STARTED = 1,
	SHUTDOWN,
	CONNECTION_CREATED,
	ACCEPT_FAILED,
	SELECT_INTR,
	SELECT_ARG_ERR,
	SELECT_TIMEOUT,
	WRITE_ERR,
	READ_ERR,
	CONNECTION_CLOSED,
	FILE_OPEN_ERR,
	FILE_READ_ERR,
	FILE_SEEK_ERR,
	REQ_RECIEVED,
	RESP_WRITE
};

struct log_msg {
	int msg_code;
	const char *msg;
};

int log_init();
void write_log(int level, int msg_code, const char *data);
void log_close();

#endif
