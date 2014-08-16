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
	int read_fd;
};

extern struct host_data hosts[MAX_HOSTS];
extern int nhosts;

void parse_children_output();

#endif
