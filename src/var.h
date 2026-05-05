#ifndef X__VAR_H__X
#define X__VAR_H__X
#include <stddef.h>

#include "oper.h"

/*
*	TODO
* Make contents a hashmap.
* Allocate keys statically.
*/

struct var {
	char *key, *value;
};
struct variables {
	struct var *contents;
	size_t length, capacity;
};

void
create_variables(OUT CREATED struct variables *ret);

void
destroy_variables(IN DESTROYED struct variables *ret);

void
set_var(IN OUT struct variables *vars, IN const char *key,
IN const char *value);

const char *
get_var(IN OUT struct variables *vars, IN const char *key);

#endif
