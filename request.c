#include <stdio.h>
#include <string.h>

size_t create_response_header(char* output, size_t output_len)
{
	const char* header = "HTTP/1.1 200 OK\r\n"
                 "Content-Type: text/html\r\n"
                 "\r\n";
	size_t header_len = strlen(header);

	if (output_len <= header_len) {
		fprintf(stderr, "Erro! Output len is less than header len.\n");
		return 0;
	}
	strcpy(output, header);

	return header_len;
}
