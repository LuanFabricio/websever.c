#include "handlers.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <regex.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define __HOTRELOAD_IMPLEMENTATION
#include "hotreload.h"
#include "request.h"

#define PORT 4242
#define BUFFER_SIZE 1024

static void* request_ptr = NULL;
create_response_header_t create_response_header;
void reset_request()
{
	const char* file_path = "./request.so";

	request_ptr = hr_reset_file(file_path, request_ptr);

	create_response_header = hr_reset_function(request_ptr, "create_response_header");
}

static void* handlers_ptr = NULL;
handle_request_t handle_request;
void reset_handlers()
{
	const char* file_path = "./handlers.so";

	handlers_ptr = hr_reset_file(file_path, handlers_ptr);

	handle_request = hr_reset_function(handlers_ptr, "handle_request");
}

int main(void)
{
	reset_func_t functions[] = {
		&reset_request, &reset_handlers
	};
	hr_init(CALC_SIZEOF(functions), functions);

	int server_fd = socket(AF_INET, SOCK_STREAM, 0);

	if (server_fd < 0) {
		fprintf(stderr, "[Error] On initializing a socket: %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}

	struct sockaddr_in server_addr = {0};

	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port = htons(PORT);

	size_t socket_len = sizeof(server_addr);

	if (bind(server_fd, (struct sockaddr*)&server_addr, socket_len) < 0) {
		fprintf(stderr, "[Error] On binding socket: %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}


	if (listen(server_fd, 3) < 0) {
		fprintf(stderr, "[Error] On listening the socket: %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}



	while (1) {
		struct sockaddr_in client_addr = {0};
		socklen_t sock_client_len = sizeof(client_addr);

		int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &sock_client_len);
		hr_reset_all();

		if (client_fd < 0) {
			fprintf(stderr, "[Error] On accepting a connection: %s\n", strerror(errno));
			continue;
		}

		handle_request(client_fd, create_response_header);
	}

	close(server_fd);
	return EXIT_SUCCESS;
}
