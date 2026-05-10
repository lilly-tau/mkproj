#ifndef X__PARSE_H__X
#define X__PARSE_H__X
#include "oper.h"
#include "var.h"

#define EOLINE "\n"
#define EOLINE_LEN 2
#define EOTOK ", \t"

struct parser {
	const char *src;
	char *token;
	size_t index, tindex, token_capacity, next, srclen, line;
	BOOLEAN is_str;
};

void
create_parser(OUT CREATED struct parser *ret, SHARED const char *src);

void
destroy_parser(IN DESTROYED struct parser *ret);

BOOLEAN
read_line(IN OUT struct parser *ret);

BOOLEAN
read_tokens(IN OUT struct parser *ret, struct variables *vars);

BOOLEAN
is_identifier(IN const char *string);

#endif
