#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "oper.h"

void
p_assert(BOOLEAN condition, BOOLEAN fatal, const char *fmt, ...)
{
	va_list args;

	if (!condition) {
		va_start(args, fmt);
		vfprintf(stderr, fmt, args);
		va_end(args);

		if (fatal)
			exit(1);
	}
}

BOOLEAN
read_file(const char *path, char **ret)
{
	FILE *file;
	size_t length;

	file = fopen(path, "r");
	if (file == NULL)
		return FALSE;

	fseek(file, 0L, SEEK_END);
	length = ftell(file);
	*ret = malloc(length + 1);
	fseek(file, 0L, SEEK_SET);

	length = fread(*ret, 1, length, file);
	*ret = realloc(*ret, length + 1);
	(*ret)[length] = 0;

	return TRUE;
}
