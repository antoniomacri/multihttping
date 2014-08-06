/*
 * multihost.c
 *
 * Authors: Sara Cadau, Antonio Macrì
 */

#include <errno.h>
#include <unistd.h>

#include "multihost.h"
#include "utils.h"

struct host_data hosts[MAX_HOSTS];
int nhosts = 0;

static int parse_child_output(int i, int fd)
{
	char buffer[1024];
	int n = read(fd, buffer, 1024);
	if (n > 0)
	{
		write(STDOUT_FILENO, buffer, n);
	}
	return n;
}

void parse_children_output()
{
	int i;

	while (1)
	{
		int max_fd = 0;
		fd_set read_set;
		FD_ZERO(&read_set);

		for (i = 0; i < nhosts; i++)
		{
			if (hosts[i].read_fd >= 0)
			{
				FD_SET(hosts[i].read_fd, &read_set);
				max_fd = max(max_fd, hosts[i].read_fd);
			}
		}

		if (max_fd <= 0)
			break;

		if (select(max_fd + 1, &read_set, NULL, NULL, NULL) < 0)
		{
			if (errno == EINTR)
				continue;
		}

		for (i = 0; i < nhosts; i++)
		{
			if (FD_ISSET(hosts[i].read_fd, &read_set))
			{
				if (!parse_child_output(i, hosts[i].read_fd))
				{
					// Child terminated.
					close(hosts[i].read_fd);
					hosts[i].read_fd = -1;
				}
			}
		}
	}
}
