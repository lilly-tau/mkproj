#include "ctype.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parse.h"
#include "oper.h"
#include "var.h"

void
create_parser(struct parser *ret, const char *src)
{
	memset(ret, 0, sizeof(*ret));
	ret->src = malloc(strlen(src) + 1);
	strcpy(ret->src, src);
	ret->srclen = strlen(ret->src);

	ret->token = malloc(ret->token_capacity = 0x200);
}

void
destroy_parser(struct parser *ret)
{
	free(ret->src);
	free(ret->token);
}

BOOLEAN
is_identifier(const char *string)
{
	size_t i;

	for (i = 0; string[i]; i++) {
		if (!isalpha(string[i]) && string[i] != '_')
			break;
	}

	return !(string[i]);
}

static void
s_add_region(struct parser *ret, const char *memory, size_t length)
{
	if (strlen(ret->token) + length > ret->token_capacity) {
		ret->token_capacity = 1 + (strlen(ret->token) + length)
			/ 0x200 * 0x200;
		ret->token = realloc(ret->token, ret->token_capacity);
	}

	ret->token[strlen(ret->token) + length] = 0;
	memcpy(ret->token + strlen(ret->token), memory, length);
}

BOOLEAN
read_line(struct parser *ret)
{
	size_t place, i;
	const char *nextptr[EOLINE_LEN];

	ret->token[0] = 0;

	if (ret->next > 0)
		ret->index += ret->next;

	if (!ret->src[ret->index])
		return FALSE;

	while (ret->src[ret->index] && isspace(ret->src[ret->index]))
		ret->index += 1;

	if (ret->src[ret->index] == 0)
		return FALSE;

	ret->next = ~(size_t)0;
	for (i = 0; i < EOLINE_LEN; i++) {
		nextptr[i] = strchr(ret->src + ret->index, EOLINE[i]);
		nextptr[i] = OPTION(nextptr[i], ret->src + ret->srclen);
		ret->next = MIN(ret->next, nextptr[i] - (ret->src + ret->index));
	}

	if (ret->src[ret->index + ret->next] == '\n')
		ret->line += 1;

	for (place = ret->index;
	ret->index - place < ret->next && !isspace(ret->src[ret->index]);
	ret->index++);

	if (ret->index == place)
		return FALSE;

	ret->token[0] = 0;
	s_add_region(ret, ret->src + place, ret->index - place);

	ret->tindex = ret->index;
	ret->next -= ret->index - place;
	return TRUE;
}

BOOLEAN
read_token(struct parser *ret, struct variables *vars) {
	size_t place, count;
	char c, num[3] = {0};
	const char *value;
	long tmp;
	char *eptr;

	ret->token[0] = 0;


	while (ret->tindex - ret->index < ret->next
	&& ret->src[ret->tindex]
	&& (isspace(ret->src[ret->tindex])
	|| strchr(EOTOK, ret->src[ret->tindex]) != NULL))
		ret->tindex += 1;

	if (ret->tindex - ret->index >= ret->next)
		return FALSE;

	if (ret->src[ret->tindex] == '\"') {
		for (place = ++ret->tindex, count = 0;
		ret->tindex - ret->index < ret->next
		&& ret->src[ret->tindex] != '\"';
		ret->tindex++, count++) {
			switch (ret->src[ret->tindex]) {
			case '\\':
				ret->tindex += 1;
				switch (ret->src[ret->tindex]) {
				case 'n': c = '\n'; break;
				case 'r': c = '\r'; break;
				case 't': c = '\t'; break;
				case 'x':
					num[0] = ret->src[ret->tindex + 1];
					num[1] = ret->src[ret->tindex + 2];
					ret->tindex += 2;
					tmp = strtol(num, &eptr, 0x10);
					p_assert(*eptr != 0, TRUE,
						"Invalid \\x in string on"
						" line %u.\n",
						ret->line);
					c = *(char*)&tmp;
					break;
				default:
					c = ret->src[ret->tindex];
					break;
				}
				break;
			default:
				c = ret->src[ret->tindex];
				break;
			}
			s_add_region(ret, &c, 1);
		}

		ret->tindex += 1;

		return TRUE;
	} else if (ret->src[ret->tindex] == '$') {
		if (ret->src[ret->tindex + 1] == '$') {
			strcpy(ret->token, "$");
			return TRUE;
		}
		for (place = ++ret->tindex;
		ret->tindex - ret->index < ret->next
		&& !isspace(ret->src[ret->tindex])
		&& strchr(EOTOK, ret->src[ret->tindex]) == NULL;
		ret->tindex++);


		if (ret->tindex == place)
			return FALSE;

		ret->token[0] = 0;
		s_add_region(ret, ret->src + place, ret->tindex - place);

		value = get_var(vars, ret->token);

		ret->token[0] = 0;
		s_add_region(ret, value, strlen(value));

		return TRUE;
	}

	for (place = ret->tindex++;
	ret->tindex - ret->index < ret->next && !isspace(ret->src[ret->tindex])
	&& strchr(EOTOK, ret->src[ret->tindex]) == NULL;
	ret->tindex++);


	if (ret->tindex == place)
		return FALSE;

	ret->token[0] = 0;
	s_add_region(ret, ret->src + place, ret->tindex - place);

	return TRUE;
}
