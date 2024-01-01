#include <dlfcn.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <regex.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define __HOTRELOAD_IMPLEMENTATION
#include "hotreload.h"
#include "request.h"

#define BUFFER_SIZE 1024

void replace_last_simbol(char* str, char src, char dst)
{
	for (int i = strlen(str)-1 ; i >= 0 ; i--) {
		if (str[i] == src) {
			str[i] = dst;
			break;
		}
	}
}

static void *dynamic_file_ptr = NULL;
char* run_dynamic_page(const char *base_path, char* arg)
{
	char* tmp = malloc(sizeof(char) * strlen(base_path));
	strcpy(tmp, base_path);
	replace_last_simbol(tmp, '/', '\0');

	char* file_path = malloc(sizeof(char) * BUFFER_SIZE);
	snprintf(file_path, BUFFER_SIZE, "pages/%s/[id].so", tmp);

	dynamic_file_ptr = hr_reset_file(file_path, dynamic_file_ptr);
	// NOTE: For some reason, the function loaded inst the latest version.
	// So, if you update the pages/%s/[id].so, the next function call wont be the
	// lastest compiled version.
	char* (*index_page)(char*) = hr_reset_function(dynamic_file_ptr, "index_page");

	free(tmp);

	return index_page(arg);
}

// TODO: Just output a html static page.
void run_static_page(const char* filepath, char **output)
{
	char* tmp = malloc(sizeof(char)*(strlen(filepath)+strlen("pages.html")));
	strcpy(tmp, "pages/");
	strcat(tmp, filepath);
	strcat(tmp, ".html");

	FILE *html = fopen(tmp, "r");

	if (html == NULL) {
		fprintf(stderr, "[Error] On reading file %s: %s\n", tmp, strerror(errno));
		return;
	}

	char* buffer = malloc(sizeof(char)*BUFFER_SIZE);

	*output = malloc(sizeof(char)*BUFFER_SIZE);
	*output[0] = '\0';
	while (fgets(buffer, BUFFER_SIZE, html)) {
		strcat(*output, buffer);
	}

	free(buffer);

	fclose(html);
}

size_t count_simbol(const char *str, const char simbol)
{
	size_t n = 0;

	for (size_t i = 0; i < strlen(str); i++) {
		if (str[i] == simbol) {
			n++;
		}
	}

	return n;
}

char* replace_first(char* str, char src, char dst)
{
	for (size_t i = 0 ; i < strlen(str) ; i++) {
		if (str[i] == src) {
			str[i] = dst;
			break;
		}
	}

	return str;
}

bool file_exists(const char* path, const char *filename, char** file)
{
	DIR *d;
	struct dirent *dir;

	d = opendir(path);

	if (d == NULL) {
		fprintf(stderr, "[Error] On reading dir %s: %s\n", path, strerror(errno));
		exit(EXIT_FAILURE);
	}

	while((dir = readdir(d)) != NULL) {
		const char *dir_filename = replace_first(dir->d_name, '.', '\0');
		if (strcmp(dir_filename, filename) == 0) {
			*file = malloc(sizeof(char) * strlen(dir->d_name));
			strcpy(*file, dir->d_name);
			return true;
		}
	}

	closedir(d);

	return false;
}

enum {
	FOUND_ERROR=-1,
	FOUND_DYNAMIC=0,
	FOUND_STATIC=1,
} FindResponse;

int find_page_to_serve(const char* url, char** output)
{
	size_t url_len = strlen(url);

	if (url_len <= 0) {
		return FOUND_ERROR;
	}

	char* copy_buffer = malloc(sizeof(char)*url_len);
	char* initial_buffer = copy_buffer;
	strcpy(copy_buffer, url);

	regex_t regex;
	regcomp(&regex, "([^ ]*/)", REG_EXTENDED);
	regmatch_t matches[2];

	size_t quant_subpaths = count_simbol(url, '/');

	size_t subpath_index = 0;
	char **subpaths = malloc(sizeof(char*) * (quant_subpaths+1));

	if (regexec(&regex, url, 2, matches, 0) == 0) {
		while (subpath_index < quant_subpaths + 1) {
			const size_t subpath_len = matches[1].rm_eo - matches[1].rm_so + 1;
			subpaths[subpath_index] = malloc(sizeof(char) * subpath_len);

			strcpy(subpaths[subpath_index], copy_buffer);
			subpaths[subpath_index][subpath_len-1] = '\0';

			copy_buffer += matches[1].rm_eo;

			subpath_index++;
		}
	} else {
		return FOUND_ERROR;
	}

	char *file_path = malloc(sizeof(char) * (url_len + strlen("pages")));
	strcat(file_path, "pages/");

	for (size_t i = 0; i < subpath_index-1; i++) {
		strcat(file_path, subpaths[i]);
	}

	bool exists = file_exists(file_path, subpaths[subpath_index-1], output);

	if (!exists) {
		*output = malloc(sizeof(char) * strlen(subpaths[subpath_index-1]));
		strcpy(*output, subpaths[subpath_index-1]);
	}

	free(initial_buffer);

	for (size_t i = 0; i < subpath_index; i++) {
		free(subpaths[i]);
	}
	free(subpaths);

	return exists ? FOUND_STATIC : FOUND_DYNAMIC;
}

void handle_request(int client_fd, create_response_header_t create_response_header)
{
	char *buffer = malloc(sizeof(char)*BUFFER_SIZE);
	ssize_t bytes_received = recv(client_fd, buffer, BUFFER_SIZE, 0);

	if (bytes_received > 0) {
		regex_t regex;
		regcomp(&regex, "GET /([^ ]*) HTTP/1.1", REG_EXTENDED);
		regmatch_t matches[2];

		if (regexec(&regex, buffer, 2, matches, 0) == 0) {
			buffer[matches[1].rm_eo] = '\0';

			const char *url = buffer + matches[1].rm_so;

			char* response = malloc(sizeof(char)*BUFFER_SIZE);
			size_t padding = create_response_header(response, BUFFER_SIZE);

			snprintf(response + padding, BUFFER_SIZE, "Hello from %s!", url);

			char* output = NULL;
			int page_type = find_page_to_serve(url, &output);
			switch (page_type) {
				case FOUND_DYNAMIC:
				{
					char* res = run_dynamic_page(url, output);
					strcat(response, res);
					free(res);
				}
				break;
				case FOUND_STATIC:
				{
					run_static_page(url, &output);
					strcat(response, output);
				}
				break;
				case FOUND_ERROR:
				default:
				{
					fprintf(stderr, "Error on finding %s page\n", url);
					break;
				}
			}

			if (output) {
				free(output);
			}

			send(client_fd, response, strlen(response), 0);

			free(response);
		}

		regfree(&regex);
	}

	close(client_fd);
	free(buffer);
}
