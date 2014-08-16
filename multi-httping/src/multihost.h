/*
 * multihost.h
 *
 * Authors: Sara Cadau, Antonio Macrì
 */

#ifndef MULTIHOST_H_
#define MULTIHOST_H_

#include <sys/select.h>

#include <jansson.h>

#define MAX_HOSTS FD_SETSIZE

struct host_data
{
	char *name;
	char read_buffer[1024];
	int read_count;
	int read_fd;
};

extern struct host_data hosts[MAX_HOSTS];
extern int nhosts;
extern int hostname_max_length;

void parse_children_output();

#endif
