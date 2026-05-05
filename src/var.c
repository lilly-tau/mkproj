#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "var.h"

void
create_variables(struct variables *ret)
{
	memset(ret, 0, sizeof(*ret));
	ret->capacity = 0x10;
	ret->contents = malloc(ret->capacity * sizeof(*ret->contents));
}

void
destroy_variables(struct variables *ret)
{
	size_t i;

	for (i = 0; i < ret->length; i++) {
		free(ret->contents[i].key);
		free(ret->contents[i].value);
	}

	free(ret->contents);
}

void
set_var(struct variables *vars, const char *key, const char *value)
{
	size_t i;

	for (i = 0; i < vars->length; i++) {
		if (!strcmp(vars->contents[i].key, key))
			break;
	}

	if (i < vars->length) {
		vars->contents[i].value = realloc(vars->contents[i].value,
			strlen(value) + 1);
		strcpy(vars->contents[i].value, value);
		return;
	}

	vars->length += 1;
	if (vars->length > vars->capacity) {
		vars->capacity += 0x10;
		vars->contents = realloc(vars->contents,
			vars->capacity * sizeof(*vars->contents));
	}
	vars->contents[vars->length - 1].key = malloc(strlen(key) + 1);
	vars->contents[vars->length - 1].value = malloc(strlen(value) + 1);
	strcpy(vars->contents[vars->length - 1].value, value);
	strcpy(vars->contents[vars->length - 1].key, key);
}

const char *
get_var(struct variables *vars, const char *key)
{
	size_t i;

	for (i = 0; i < vars->length; i++) {
		if (!strcmp(vars->contents[i].key, key))
			break;
	}

	if (i == vars->length)
		return "";

	return vars->contents[i].value;
}

