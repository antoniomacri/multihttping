/*
 * multihost.c
 *
 * Authors: Sara Cadau, Antonio Macrì
 */

#include <errno.h>
#include <string.h>
#include <unistd.h>

#include "float.h"
#include "multihost.h"
#include "utils.h"

struct host_data hosts[MAX_HOSTS];
int nhosts = 0;
int hostname_max_length = 0;

void initialize_host(struct host_data *host, const char *name, int read_fd)
{
	host->name = name;
	host->read_fd = read_fd;
}

static void update_host_time(const json_t *root, const char *js_name, struct host_time *time,
		char *buffer, int len)
{
	const char *node_value_s = json_string_value(json_object_get(root, js_name));
	if (node_value_s != NULL)
	{
		double value = atof(node_value_s);
		time->sum += value;
		if (value < time->min || time->min == 0)
			time->min = value;
		if (value > time->max)
			time->max = value;
		time->count++;
		snprintf(buffer, len, "%.2lf", value);
	}
	else
	{
		snprintf(buffer, len, "?");
	}
}

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

		char buffer[1024];
		int count = 0;
		json_t *node;

		const char *node_value_s = json_string_value(json_object_get(root, "host"));
		count += snprintf(buffer + count, sizeof(buffer) - count, "%-*s  %-15s", hostname_max_length,
				h->name, (node_value_s != NULL ? node_value_s : "-"));

		node = json_object_get(root, "seq");
		node_value_s = json_is_string(node) ? json_string_value(node) : "?";
		count += snprintf(buffer + count, sizeof(buffer) - count, " %5s", node_value_s);

		node = json_object_get(root, "status");
		node_value_s = json_is_string(node) ? json_string_value(node) : "?";
		int ok = atoi(node_value_s);
		count += snprintf(buffer + count, sizeof(buffer) - count, " %3s", node_value_s);

		if (ok)
		{
			char buff1[20], buff2[20];
			node = json_object_get(root, "header_size");
			snprintf(buff1, sizeof(buff1), "%s", json_is_string(node) ? json_string_value(node) : "?");
			node = json_object_get(root, "data_size");
			snprintf(buff2, sizeof(buff2), "%s+%s",
					buff1, json_is_string(node) ? json_string_value(node) : "?");
			count += snprintf(buffer + count, sizeof(buffer) - count, " %10s", buff2);

			if (multihost_options.split)
			{
				char resolve_s[40], connect_s[40], write_s[40], request_s[40], close_s[40];

				update_host_time(root, "resolve_ms", &h->resolve, resolve_s, sizeof(resolve_s));
				update_host_time(root, "connect_ms", &h->connect, connect_s, sizeof(connect_s));
				update_host_time(root, "write", &h->write, write_s, sizeof(write_s));
				update_host_time(root, "request_ms", &h->request, request_s, sizeof(request_s));
				update_host_time(root, "close", &h->close, close_s, sizeof(close_s));

				count += snprintf(buffer + count, sizeof(buffer) - count, "  %s+%s+%s+%s+%s",
						resolve_s, connect_s, write_s, request_s, close_s);
			}

			char total_s[40];
			update_host_time(root, "total_ms", &h->total, total_s, sizeof(total_s));
			count += snprintf(buffer + count, sizeof(buffer) - count, " %s %s",
					(multihost_options.split ? "=" : ""), total_s);

			printf("%s\n", buffer);

			h->packets_received++;
		}
		else
		{
			node_value_s = json_string_value(json_object_get(root, "msg"));
			count += snprintf(buffer + count, sizeof(buffer) - count, "  %s", node_value_s);

			h->packets_failed++;
		}
		printf("%s\n", buffer);

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
	printf("%-*s  Received  Failed  Time (min/avg/max) [ms]\n", hostname_max_length, "Host");

	int i;
	for (i = 0; i < nhosts; i++)
	{
		struct host_data * const h = &hosts[i];
		char buffer[1024];
		int count = 0;

		count += snprintf(buffer + count, sizeof(buffer) - count, "%-*s", hostname_max_length, h->name);

		count += snprintf(buffer + count, sizeof(buffer) - count, " %9d", h->packets_received);
		count += snprintf(buffer + count, sizeof(buffer) - count, " %7d", h->packets_failed);

		if (h->total.count > 0)
		{
			count += snprintf(buffer + count, sizeof(buffer) - count, "  %.2lf", h->total.min);
			count += snprintf(buffer + count, sizeof(buffer) - count, "/%.2lf",
					h->total.sum / h->total.count);
			count += snprintf(buffer + count, sizeof(buffer) - count, "/%.2lf", h->total.max);
		}
		else
		{
			count += snprintf(buffer + count, sizeof(buffer) - count, "  -/-/-");
		}

		printf("%s\n", buffer);
	}
}
