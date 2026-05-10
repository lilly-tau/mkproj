#include <getopt.h>
#include <libgen.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "oper.h"
#include "parse.h"
#include "var.h"

#define OUTPUT_COUNT 0x03


BOOLEAN
expr_exec(IN OUT struct parser *parser, IN OUT struct variables *vars,
IN const char *identifier, IN const char *file_path)
{
	char *tokens[2];

	p_assert(read_token(parser, vars), TRUE, "Expression expects"
		" operation on line %u (of file %s).\n", parser->line,
		file_path);

	if (!strcmp("eq", parser->token)) {
		p_assert(read_token(parser, vars), TRUE, "eq expects two"
			" arguments, got zero on line %u (of file %s).\n",
			parser->line, file_path);
		tokens[0] = malloc(strlen(parser->token) + 1);
		strcpy(tokens[0], parser->token);
		p_assert(strchr(tokens[0], '\x03') == NULL, TRUE,
			"eq expects atomic values on line %u (of file %s)",
			parser->line, file_path);

		p_assert(read_token(parser, vars), TRUE, "eq expects two"
			" arguments, got one on line %u (of file %s).\n",
			parser->line, file_path);
		tokens[1] = malloc(strlen(parser->token) + 1);
		strcpy(tokens[1], parser->token);
		p_assert(strchr(tokens[1], '\x03') == NULL, TRUE,
			"eq expects atomic values on line %u (of file %s)",
			parser->line, file_path);

		set_var(vars, identifier,
			!strcmp(tokens[0], tokens[1])
			? "true"
			: "");

		free(tokens[0]);
		free(tokens[1]);
	} else if (!strcmp("not", parser->token)) {
		p_assert(read_token(parser, vars), TRUE, "not expects an"
			" arguments, got none on line %u (of file %s).\n",
			parser->line, file_path);
		tokens[0] = malloc(strlen(parser->token) + 1);
		strcpy(tokens[0], parser->token);

		set_var(vars, identifier, strlen(tokens[0]) ? "" : "true");

		free(tokens[0]);
	} else if(!strcmp("cat", parser->token)) {
		p_assert(read_token(parser, vars), TRUE, "cat expects two"
			" arguments, got zero on line %u (of file %s).\n",
			parser->line, file_path);
		tokens[0] = malloc(strlen(parser->token) + 1);
		strcpy(tokens[0], parser->token);

		p_assert(read_token(parser, vars), TRUE, "cat expects two"
			" arguments, got one on line %u (of file %s).\n",
			parser->line, file_path);
		tokens[0] = realloc(tokens[0], strlen(tokens[0])
			+ strlen(parser->token) + 1);
		strcat(tokens[0], parser->token);
		set_var(vars, identifier, tokens[0]);
		free(tokens[0]);
	} else if (!strcmp("curdir", parser->token)) {
		tokens[0] = malloc(strlen(file_path) + 1);
		strcpy(tokens[0], file_path);
		set_var(vars, identifier, dirname(tokens[0]));
		free(tokens[0]);
	} else if (!strcmp("atom", parser->token)) {
		p_assert(read_token(parser, vars), TRUE, "atom expects one"
			" argument, got zero on line %u (of file %s).\n",
			parser->line, file_path);
		set_var(vars, identifier,
			strchr(parser->token, '\x03') == NULL
			? "true"
			: "");
	} else if (!strcmp("car", parser->token)) {
		p_assert(read_token(parser, vars), TRUE, "car expects one"
			" argument, got zero on line %u (of file %s).\n",
			parser->line, file_path);
		p_assert(strchr(parser->token, '\x03') != NULL, TRUE,
			"car expects list argument on line %u (of file %s)\n",
			parser->line, file_path);
		tokens[0] = malloc(strlen(parser->token) + 1);
		strcpy(tokens[0], parser->token);

		set_var(vars, identifier, strtok(tokens[0], "\x03"));
		free(tokens[0]);
	} else if (!strcmp("cdr", parser->token)) {
		p_assert(read_token(parser, vars), TRUE, "cdr expects one"
			" argument, got zero on line %u (of file %s).\n",
			parser->line, file_path);
		p_assert(strchr(parser->token, '\x03') != NULL, TRUE,
			"car expects list argument on line %u (of file %s)\n",
			parser->line, file_path);

		set_var(vars, identifier, strchr(parser->token, '\x03') + 1);
	} else if (!strcmp("cons", parser->token)) {
		p_assert(read_token(parser, vars), TRUE, "cons expects two"
			" arguments, got zero on line %u (of file %s).\n",
			parser->line, file_path);
		tokens[0] = malloc(strlen(parser->token) + 1);
		strcpy(tokens[0], parser->token);

		p_assert(read_token(parser, vars), TRUE, "cons expects two"
			" arguments, got one on line %u (of file %s).\n",
			parser->line, file_path);
		tokens[0] = realloc(tokens[0], strlen(tokens[0])
			+ strlen(parser->token) + 2);
		strcat(tokens[0], "\x03");
		strcat(tokens[0], parser->token);

		set_var(vars, identifier, tokens[0]);

		free(tokens[0]);
	} else if (!strcmp("\\", parser->token)) {
		p_assert(read_token(parser, vars), TRUE, "set expr expects"
			" two arguments got none on line %u (of file %s).\n",
			parser->line, file_path);
		tokens[0] = malloc(strlen(parser->token) + 1);
		strcpy(tokens[0], parser->token);

		p_assert(is_identifier(tokens[0]), TRUE, "set expr expects an"
			" identifier as the first argument, got '%s' on line"
			" %u (of file %s)", tokens[0], parser->line,
			file_path);

		p_assert(read_token(parser, vars), TRUE, "set expr expects "
			"two arguments got one on line %u (of file %s).\n",
			parser->line, file_path);

		if (!strcmp("(", parser->token))
			while (expr_exec(parser, vars, tokens[0], file_path));
		else
			set_var(vars, tokens[0], parser->token);

		free(tokens[0]);
	} else if (!strcmp(")", parser->token)) {
		return FALSE;
	} else {
		set_var(vars, identifier, parser->token);
	}

	return TRUE;
}

void
config_exec(IN const char *origin_src, IN const char *type,
IN OUT struct variables *vars, const char *file_path)
{
	size_t index, length;
	FILE *const outputs[OUTPUT_COUNT] = {stdin, stdout, stderr};
	FILE *file;
	unsigned char output_index;
	char pipe_flags[0x10];
	char number[0x20];
	char *identifier, *value;
	const char *tmpstr;
	BOOLEAN first;
	struct parser parser;
	struct {
		char *contents;
		size_t capacity, length;
	} input = {0};
	size_t return_place;
	char *src;

	src = malloc(strlen(origin_src) + 1);
	strcpy(src, origin_src);

	input.contents = malloc(0x200);
	input.capacity = 0x200;

	create_parser(&parser, src);

	if (strcmp(type, "")) {
		identifier = malloc(strlen(type) + 4);
		sprintf(identifier, "@@%s:", type);

		tmpstr = strstr(parser.src, identifier);
		p_assert(tmpstr != NULL, TRUE, "Could not find type '%s' on"
			" line %u (of file %s)", type, parser.line, file_path);
		parser.index = tmpstr - parser.src + strlen(identifier) + 1;

		parser.tindex = parser.index;
		parser.next = 0;
		free(identifier);
	}

	while (read_line(&parser)) {
		switch (parser.token[0]) {
		case '>':
			if (strlen(parser.token) > 1) {
				output_index =
					*(unsigned char*)&parser.token[1]
					- 0x30;

				if (output_index >= OUTPUT_COUNT)
					output_index = 0x01;
			} else {
				output_index = 0x01;
			}

			index = (parser.src[parser.index] == ' '
				|| parser.src[parser.index] == '\t');

			while (read_token(&parser, vars)) {
				value = malloc(strlen(parser.token) + 1);
				strcpy(value, parser.token);

				tmpstr = strtok(value, "\x03");
				first = TRUE;
				while (tmpstr != NULL) {
					if (!first) {
						fprintf(outputs[output_index],
							", ");
					}
					first = FALSE;
					fprintf(outputs[output_index], "%s",
						tmpstr);
					tmpstr = strtok(NULL, "\x03");
				}

			}

			fflush(outputs[output_index]);

			break;
		case '<':
			p_assert(read_token(&parser, vars), TRUE,
				"Input comand expects variable identifier,"
				" got end of line %u (of file %s).\n",
				parser.line, file_path);

			p_assert(is_identifier(parser.token), TRUE,
				"Input command expects variable identifier,"
				" got '%s' on line %u (file %s).\n",
				parser.token, parser.line, file_path);

			identifier = malloc(strlen(parser.token) + 1);
			strcpy(identifier, parser.token);

			input.length = 0;
			do {
				input.length = strlen(fgets(
					input.contents + input.length,
					input.capacity - input.length,
					stdin));
				if (input.length == input.capacity) { input.capacity >>= 1;
					input.contents = realloc(
						input.contents,
						input.capacity);
				}
			} while (input.contents[input.length - 1] != '\n');
			input.contents[input.length - 1] = 0;

			set_var(vars, identifier, input.contents);

			free(identifier);
			break;
		case '?':
			if (!strcmp(parser.token + 1, "(")) {
				while(expr_exec(&parser, vars, "_c",
				file_path));
				tmpstr = get_var(vars, "_c");
			} else {
				tmpstr = get_var(vars, parser.token + 1);
			}
			p_assert(read_token(&parser, vars), TRUE,
				"Conditional expects label on line %u (of"
				" file %s).", parser.line, file_path);

			if (!*tmpstr) break;
		case '%':
	goto_command:
			if (parser.token[1] == '$') {
				identifier = malloc(strlen(
					get_var(vars, parser.token + 2)) + 3);
				sprintf(identifier, "@%s:",
					get_var(vars, parser.token + 2));
			} else {
				identifier = malloc(strlen(parser.token) + 3);
				sprintf(identifier, "@%s:", parser.token + 1);
			}

			if (identifier[1] == '.') {
				tmpstr = strstr(
					strstr(parser.src,
					get_var(vars, "_pfx")),
					identifier);
			} else
				tmpstr = strstr(parser.src, identifier);

			p_assert(tmpstr != NULL, TRUE, "Could not find"
				" label '%s' on line %u (of file %s)",
				parser.token + 1, parser.line, file_path);
			parser.index = tmpstr - parser.src;

			parser.tindex = parser.index;
			parser.next = 0;
			free(identifier);
			break;
		case '@':
			if (parser.token[1] == '@')
				return;
			else if((tmpstr = strchr(parser.token, ':')) != NULL
			&& parser.token[1] != '.') {
				value = malloc(tmpstr - parser.token + 1);
				memcpy(value, parser.token,
					tmpstr - parser.token);
				value[tmpstr - parser.token] = 0;
				set_var(vars, "_pfx", value);
			}
			break;
		case '~':
			p_assert(read_token(&parser, vars), TRUE,
				"Mkdir instruction expects directory on"
				" line %u (of file %s).\n", parser.line,
				file_path);
			mkdir(parser.token, 0777);
			break;
		case '|':
			strcpy(pipe_flags, parser.token + 1);
			if (strchr(pipe_flags, '|') != NULL)
				file = fopen(get_var(vars, "output"), "w");
			else
				file = fopen(get_var(vars, "output"), "a");

			p_assert(file != NULL, TRUE,
				"Could not create file %s on line %u"
				" (of file %s).\n", get_var(vars, "output"),
				parser.line, file_path);

			if (strchr(pipe_flags, '$') != NULL) {
				p_assert(read_token(&parser, vars), TRUE,
					"%s requires a variable as its first"
					" argument on line %u (of file %s).\n",
					parser.token, parser.line, file_path);

				fprintf(file, "%s", get_var(vars,
					parser.token));
			} else if(parser.src[parser.index] == ' '
			|| parser.src[parser.index] == '\t') {
				fprintf(file, "%.*s", parser.next - 1,
					parser.src + parser.index + 1);
			} else {
				fprintf(file, "%.*s", parser.next,
					parser.src + parser.index);
			}
			
			if (strchr(pipe_flags, '&') == NULL) {
				fprintf(file, "\n");
			}

			fclose(file);
			break;
		case '\\':
			p_assert(strlen(parser.token) == 1, TRUE,
				"A space must delimit the set command '\\'"
				" and the variable name on line %u (of"
				" file %s).\n", parser.line, file_path);

			p_assert(read_token(&parser, vars), TRUE,
				"Set comand expects variable identifier, got"
				" end of line %u (of file %s).\n",
				parser.line, file_path);

			p_assert(is_identifier(parser.token), TRUE,
				"Set command expects variable identifier, got"
				" '%s' on line %u (of file %s).\n",
				parser.token, parser.line, file_path);

			identifier = malloc(strlen(parser.token) + 1);
			strcpy(identifier, parser.token);

			p_assert(read_token(&parser, vars), TRUE,
				"Set command expects value, got end of"
				" line %u (of file %s).", parser.line,
				file_path);

			if (!strcmp(parser.token, "("))
				while(expr_exec(&parser, vars, identifier,
				file_path));
			else
				set_var(vars, identifier, parser.token);

			free(identifier);
			break;
		case '#':
			if (!strcmp(parser.token + 1, "execute")) {
				p_assert(read_token(&parser, vars), TRUE,
					"Execute requires a path to read on"
					" line %u (of file %s).\n",
					parser.line, file_path);
				file = fopen(parser.token, "r");
				p_assert(file != NULL, TRUE,
					"Could not open file %s for execution,"
					" on line %u (of file %s).\n",
					parser.token, parser.line, file_path);

				fseek(file, 0L, SEEK_END);
				length = ftell(file);
				value = calloc(1, length + 1);
				fseek(file, 0L, SEEK_SET);
				length = fread(value, 1, length, file);
				value = realloc(value, length);
				value[length] = 0;
				fclose(file);

				config_exec(value, "", vars, parser.token);

				free(value);
			} else if (!strcmp(parser.token + 1, "include")) {
				p_assert(read_token(&parser, vars), TRUE,
					"Include requires a path to read on"
					" line %u (of file %s).\n",
					parser.line, file_path);
				file = fopen(parser.token, "r");
				p_assert(file != NULL, TRUE,
					"Could not open file %s for include,"
					" on line %u (of file %s).\n",
					parser.token, parser.line, file_path);

				fseek(file, 0L, SEEK_END);
				length = ftell(file);
				value = calloc(1, length + 1);
				fseek(file, 0L, SEEK_SET);
				length = fread(value, 1, length, file);
				value = realloc(value, length);
				value[length] = 0;
				fclose(file);

				src = realloc(src, strlen(src)
					+ strlen(value) + 1);

				memmove(src + parser.index + strlen(value),
					src + parser.index,
					strlen(src + parser.index));
				memcpy(src + parser.index, value,
					strlen(value));

				config_exec(value, "", vars, parser.token);

				free(value);
			} else if (!strcmp(parser.token + 1, "call")) {
				p_assert(read_token(&parser, vars), TRUE,
					"Call expects a goto on line %u (of"
					" file %s)", parser.line, file_path);
				sprintf(number, "%u",
					parser.index + parser.next);
				set_var(vars, "return", number);
				goto goto_command;
			} else if (!strcmp(parser.token + 1, "return")) {
				parser.index = strtoul(get_var(vars, "return"),
					NULL, 0x0A);
				parser.tindex = parser.index;
				parser.next = 0;
			}
			break;
		default:
			p_assert(FALSE, TRUE,
				"Invalid command '%s' on line %u (of"
				" file %s).\n", parser.token, parser.line,
				file_path);
			break;
		}
	}

	destroy_parser(&parser);
}

#define DEFAULT_CONFIG ".mkproj"
int
main(int argc, char **argv)
{
	char *type = NULL;
	char *config_path = NULL;
	char *config;
	char *identifier, *value;
	int opt;
	struct variables vars;

	create_variables(&vars);
	while ((opt = getopt(argc, argv, "c:D:t:")) > 0) {
		switch (opt) {
		case 'c':
			config_path = malloc(strlen(optarg) + 1);
			strcpy(config_path, optarg);
			break;
		case 'D':
			identifier = malloc(strlen(optarg) + 1);
			strcpy(identifier, optarg);
			p_assert(strtok(identifier, "=") != NULL, TRUE,
				"Format of -D is <identifier>=<value>.\n");
			value = strtok(NULL, "=");
			if (value == NULL)
				value = "";
			set_var(&vars, identifier, value);
			free(identifier);
			break;
		case 't':
			type = malloc(strlen(optarg) + 2);
			sprintf(type, ":%s", optarg);
			strcpy(type, optarg);
			break;
		}
	}

	p_assert(type != NULL, TRUE, "No type specified.\n");
	if (config_path == NULL) {
		config_path = malloc(strlen(getenv("HOME"))
			+ strlen(DEFAULT_CONFIG) + 1);
		sprintf(config_path, "%s/%s", getenv("HOME"), DEFAULT_CONFIG);
	}

	p_assert(read_file(config_path, &config), TRUE,
		"Could not read config (%s).\n", config_path);

	config_exec(config, type, &vars, config_path);

	destroy_variables(&vars);
	free(config_path);
	free(type);
}
