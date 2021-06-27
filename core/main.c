#include "../set.h"
#include "base.h"
#include "log.h"
#include <stdlib.h>
#include <string.h>

int get_port(char *port)
{
	if(!port)
		return 80;
	return atoi(port);
}

int main(int argc, char **argv)
{
	int port, res, ls, log_level, curr_arg = 1;
	struct host_node *hosts;
	if(argc >= 2 && 0 == strcmp(argv[curr_arg], "-d")) {
		log_level = LOG_DEBUG;
		curr_arg++;
	} else {
		log_level = LOG_INFO;
	}
	
	res = log_init(log_level);
	if(res == -1)
		return 1;

	port = get_port(argv[curr_arg]);
	ls = init_listening(port);
	if(ls == -1)
		return 1;
	hosts = set_hosts();
	run(ls, hosts);
	log_close();
	return 0;
}


