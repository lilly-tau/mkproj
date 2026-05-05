#ifndef X__OPER_H__X
#define X__OPER_H__X

typedef unsigned char BOOLEAN;
#define TRUE 0x01
#define FALSE 0x00

#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define OPTION(ptr, elseptr) ((ptr) != NULL ? (ptr) : (elseptr))
#define OUT		/* output ptr */
#define IN		/* input ptr */
#define MALLOC		/* must be freed by the user */
#define REALLOC		/* must have been allocated by the user */
#define CREATED		/* must be destroyed */
#define DESTROYED	/* must have been created */

void
p_assert(BOOLEAN condition, BOOLEAN fatal, IN const char *fmt, ...);

BOOLEAN
read_file(IN const char *path, OUT MALLOC char **ret);

#endif
