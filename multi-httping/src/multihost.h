/*
 * multihost.h
 *
 * Authors: Sara Cadau, Antonio Macrì
 */

#ifndef MULTIHOST_H_
#define MULTIHOST_H_

#include <sys/select.h>

#include <jansson.h>

#define MAX_HOSTS ((int)FD_SETSIZE)

struct host_time
{
	double min;
	double max;
	double sum;
	int count;
};

struct host_data
{
	const char *name;
	char read_buffer[1024];
	int read_count;
	int read_fd;
	struct host_time resolve;
	struct host_time connect;
	struct host_time write;
	struct host_time request;
	struct host_time close;
	struct host_time total;
	int packets_received;
	int packets_failed;
};

struct
{
	int split;
} multihost_options;

extern struct host_data hosts[MAX_HOSTS];
extern int nhosts;
extern int hostname_max_length;

void initialize_host(struct host_data *host, const char *name, int read_fd);

void parse_children_output();

void show_statistics();

#endif
