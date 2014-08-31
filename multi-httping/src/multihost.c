/*
 * multihost.c
 *
 * Authors: Sara Cadau, Antonio Macrì
 */

#include <errno.h>
#include <string.h>
#include <unistd.h>

#include "multihost.h"
#include "utils.h"

struct host_data hosts[MAX_HOSTS];
int nhosts = 0;
int hostname_max_length = 0;

//stats
stats hstat[MAX_HOSTS];

static int parse_child_output(int i)
{
	struct host_data * const h = &hosts[i];

	int n = read(h->read_fd, h->read_buffer + h->read_count, sizeof(h->read_buffer) - h->read_count - 1);
	if (n <= 0)
	{
		return 0;
	}
	h->read_count += n;
	h->read_buffer[h->read_count] = '\0';

	while (1)
	{
		char * b = strchr(h->read_buffer, '{');
		if (b == NULL)
		{
			if (h->read_count >= (int) sizeof(h->read_buffer) - 1)
			{
				// The buffer is full and no '{' was found: erase the buffer.
				h->read_count = 0;
				fprintf(stderr, "Discarding output from child %d: >%s<\n", i, h->read_buffer);
			}
			return 1;
		}

		json_error_t error;
		json_t *root = json_loads(b, JSON_DISABLE_EOF_CHECK, &error);
		if (!root)
		{
			fprintf(stderr, "Cannot parse output from child %d: >%s<\n", i, b);
			return 1;
		}

		const char *node_value_s = json_string_value(json_object_get(root, "host"));
		if (node_value_s != NULL)
		{
			char buffer[1024];
			int count = 0;
			json_t *node;

			count += snprintf(buffer + count, sizeof(buffer) - count, "%-*s  %-15s", hostname_max_length,
					h->name, node_value_s);

			node = json_object_get(root, "seq");
			node_value_s = json_is_string(node) ? json_string_value(node) : "?";
			count += snprintf(buffer + count, sizeof(buffer) - count, " %5s", node_value_s);

			node = json_object_get(root, "status");
			node_value_s = json_is_string(node) ? json_string_value(node) : "?";
			count += snprintf(buffer + count, sizeof(buffer) - count, " %3s", node_value_s);

			char buff1[20], buff2[20];
			node = json_object_get(root, "header_size");
			strncpy(buff1, json_is_string(node) ? json_string_value(node) : "?", sizeof(buff1));
			node = json_object_get(root, "data_size");
			snprintf(buff2, sizeof(buff2), "%s+%s", buff1,
			json_is_string(node) ? json_string_value(node) : "?");
			count += snprintf(buffer + count, sizeof(buffer) - count, " %10s", buff2);
			hstat[i].count++;

			if (multihost_options.split)
			{
				node_value_s = json_string_value(json_object_get(root, "resolve_ms"));
				double resolve = node_value_s != NULL ? atof(node_value_s) : 0;
				h->resolve += resolve;
				count += snprintf(buffer + count, sizeof(buffer) - count, "  %.2lf", resolve);

				node_value_s = json_string_value(json_object_get(root, "connect_ms"));
				double connect = node_value_s != NULL ? atof(node_value_s) : 0;
				h->connect += connect;
				count += snprintf(buffer + count, sizeof(buffer) - count, "+%.2lf", connect);

				node_value_s = json_string_value(json_object_get(root, "write"));
				double write = node_value_s != NULL ? atof(node_value_s) : 0;
				h->write += write;
				count += snprintf(buffer + count, sizeof(buffer) - count, "+%.2lf", write);
				node_value_s = json_string_value(json_object_get(root, "request_ms"));
				double request = node_value_s != NULL ? atof(node_value_s) : 0;
				h->request += request;
				count += snprintf(buffer + count, sizeof(buffer) - count, "+%.2lf", request);

				node_value_s = json_string_value(json_object_get(root, "close"));
				double close = node_value_s != NULL ? atof(node_value_s) : 0;
				h->close += close;
				count += snprintf(buffer + count, sizeof(buffer) - count, "+%.2lf", close);
				//calcolo min max
				double val=connect+request+write+resolve;
				hstat[i].min=connect+request+write+resolve;
				if (hstat[i].max<=val) hstat[i].max=val;
				if (hstat[i].min>=val) hstat[i].min=val;

			}

			node = json_object_get(root, "total_ms");
			double total = json_is_string(node) ? atof(json_string_value(node)) : 0;
			count += snprintf(buffer + count, sizeof(buffer) - count, " %s %.2lf",
					(multihost_options.split ? "=" : ""), total);
			printf("%s\n", buffer);
			//calcolo max min
			hstat[i].min=total;
			if (hstat[i].max<=total) hstat[i].max=total;
			if (hstat[i].min>=total) hstat[i].min=total;
			hstat[i].total+=total;
		}
		else
		{
			fprintf(stderr, "Unrecognized element skipped from child %d: >%s<\n", i, b);
		}

		h->read_count -= (b - h->read_buffer) + error.position;
		memcpy(h->read_buffer, b + error.position, h->read_count + 1);  // +1 for '\0'
	}
	return 1;
}

void parse_children_output()
{
	printf("Pinging %d hosts...\n\n", nhosts);

	printf("--- Running ---\n");
	printf("%-*s  %-15s  SeqN  OK  Num.bytes", hostname_max_length, "Host", "IP");
	if (multihost_options.split)
		printf("  Resolve+Connect+Write+Request+Close = Total [ms]\n");
	else
		printf("  RTT [ms]\n");

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

			if (hosts[i].read_fd >= 0 && FD_ISSET(hosts[i].read_fd, &read_set))
			{
				if (!parse_child_output(i))
				{
					// Child terminated.
					close(hosts[i].read_fd);
					hosts[i].read_fd = -1;
				}
			}
		}
	}
}

void show_statistics()
{
	printf("\n--- Statistics ---\n");

	int i;
	for (i = 0; i < nhosts; i++)
	{
		printf("--%s -- -- statistics--\n", hosts[i]);
		//calcolo media
		hstat[i].avg = hstat[i].total / hstat[i].count;
		printf("%u Pings\n", hstat[i].count);
		printf("MIN RTT TIME %f\n", hstat[i].min);
		printf("MAX RTT TIME %f\n", hstat[i].max);
		printf("AVG RTT TIME %f\n", hstat[i].avg);
		printf("TOTAL TIME %f\n", hstat[i].total);
	}
}
