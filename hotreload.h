#ifndef __HOTRELOAD_H
#define __HOTRELOAD_H

#include <stdlib.h>
#include <unistd.h>
#include <dlfcn.h>

#define CALC_SIZEOF(x) sizeof(x)/sizeof(x[0])

typedef void (*reset_func_t)(void);

void hr_init(size_t len, const reset_func_t *functions);
void hr_reset_all();
void hr_end();
void* hr_reset_file(const char *filepath, void* shared_ptr);
void* hr_reset_function(void* shared_ptr, const char* function_name);

#endif // __HOTRELOAD_H

#ifdef __HOTRELOAD_IMPLEMENTATION
#include <stdarg.h>

static size_t reset_functions_len;
static reset_func_t* reset_functions;

void hr_init(size_t len, const reset_func_t *functions)
{
	reset_functions_len = len;

	reset_functions = malloc(sizeof(reset_func_t) * len);

	for (size_t i = 0; i < len; i++) {
		reset_functions[i] = functions[i];
	}
}

void hr_reset_all()
{
	for (size_t i = 0; i < reset_functions_len; i++) {
		reset_functions[i]();
	}
}

void hr_end()
{
	reset_functions_len = 0;

	if (reset_functions) {
		free(reset_functions);
	}
}

void* hr_reset_file(const char *filepath, void* shared_ptr)
{
	if (shared_ptr) {
		dlclose(shared_ptr);
	}

	shared_ptr = dlopen(filepath, RTLD_NOW);

	if (shared_ptr == NULL) {
		fprintf(stderr, "[HR ERROR] could not reset the file %s: %s\n", filepath, dlerror());
		exit(1);
	}

	return shared_ptr;
}

void* hr_reset_function(void* shared_ptr, const char* function_name)
{
	void* function_ptr = dlsym(shared_ptr, function_name);

	if (function_ptr == NULL) {
		fprintf(stderr, "[HR ERROR] could not reset the function %s: %s\n", function_name, dlerror());
		exit(1);
	}

	return function_ptr;
}

#endif // __HOTRELOAD_IMPLEMENTATION
