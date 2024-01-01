#include <stdio.h>
#include <stdlib.h>

#define BUFFER_SIZE 1024

char* index_page(const char* id)
{
	char* buffer = malloc(sizeof(char) * BUFFER_SIZE);

	snprintf(buffer, BUFFER_SIZE, ""
			"<div style=\"background-color: #000000;\">"
			"<h1>Hello, %s!</h1>"
			"</div>",
			id);

	return buffer;
}
