#include "parsing.h"

// parses an argument of the command stream input
static char *
get_token(char *buf, int idx)
{
	char *tok;
	int i;

	tok = (char *) calloc(ARGSIZE, sizeof(char));
	i = 0;

	while (buf[idx] != SPACE && buf[idx] != END_STRING) {
		tok[i] = buf[idx];
		i++;
		idx++;
	}

	return tok;
}

// parses and changes stdin/out/err if needed
static bool
parse_redir_flow(struct execcmd *c, char *arg)
{
	int inIdx, outIdx;

	// flow redirection for output
	if ((outIdx = block_contains(arg, '>')) >= 0) {
		switch (outIdx) {
		// stdout redir
		case 0: {
			strcpy(c->out_file, arg + 1);
			break;
		}
		// stderr redir
		case 1: {
			strcpy(c->err_file, &arg[outIdx + 1]);
			break;
		}
		}

		free(arg);
		c->type = REDIR;

		return true;
	}

	// flow redirection for input
	if ((inIdx = block_contains(arg, '<')) >= 0) {
		// stdin redir
		strcpy(c->in_file, arg + 1);

		c->type = REDIR;
		free(arg);

		return true;
	}

	return false;
}

// parses and sets a pair KEY=VALUE
// environment variable
static bool
parse_environ_var(struct execcmd *c, char *arg)
{
	// sets environment variables apart from the
	// ones defined in the global variable "environ"
	if (block_contains(arg, '=') > 0) {
		// checks if the KEY part of the pair
		// does not contain a '-' char which means
		// that it is not a environ var, but also
		// an argument of the program to be executed
		// (For example:
		// 	./prog -arg=value
		// 	./prog --arg=value
		// )
		if (block_contains(arg, '-') < 0) {
			c->eargv[c->eargc++] = arg;
			return true;
		}
	}

	return false;
}

// Copies string from src to dest
// If src length is larger than dest it will reallocate the destination buffer,
// else provided one will be used.
//
// parameter src: Source string.
//   Must be: Not null; terminated by a null character.
// parameter dest: Destination buffer. May be reallocated, freeing the provided
// buffer address.
//   Must be: Not null; dynamically allocated memory; terminated by a null
//   character in order to determine its length.
//
// Returns: Pointer to buffer with copied value or NULL if reallocation failed
static char *
resize_and_copy_str(char *dest, char *src)
{
	size_t src_length = strlen(src);

	if (src_length > strlen(dest)) {
		dest = realloc(dest, src_length + 1);  // +1 for trailing '/0'

		if (dest == NULL) {
			return NULL;
		}
	}

	strcpy(dest, src);

	return dest;
}

// Returns global status exit code in provided buffer
// If exit code is larger than dest it will reallocate the destination buffer,
// else provided one will be used
//
// parameter dest: Destination buffer. May be reallocated, freeing the provided
// buffer address.
//   Must be: Not null; dynamically allocated memory; terminated by a null
//   character in order to not exceed its length.
static char *
get_exit_code(char *dst)
{
	// Up to 3 chars (+1 for null termination)
	// as return codes range from "0" to "255"
	char exit_code[4];
	sprintf(exit_code, "%d", status);

	return resize_and_copy_str(dst, exit_code);
}

// this function will be called for every token, and it should
// expand environment variables. In other words, if the token
// happens to start with '$', the correct substitution with the
// environment value should be performed. Otherwise the same
// token is returned. If the variable does not exist, an empty string should be
// returned within the token
//
// Hints:
// - check if the first byte of the argument contains the '$'
// - expand it and copy the value in 'arg'
// - remember to check the size of variable's value
//		It could be greater than the current size of 'arg'
//		If that's the case, you should realloc 'arg' to the new size.
static char *
expand_environ_var(char *arg)
{
	if (arg == NULL || arg[0] != ENV_VAR_START_TOKEN)
		return arg;

	char *arg_value = arg + 1;  // +1 to skip env-var token

	bool is_pseudo_var_exit_code = (strcmp(arg_value, "?") == 0);
	if (is_pseudo_var_exit_code) {
		return get_exit_code(arg);
	}

	char *env_value = getenv(arg_value);

	if (env_value == NULL) {
		arg[0] = END_STRING;
	} else {
		printf_debug("Env-var: %s expanded to %s\n", arg, env_value);

		arg = resize_and_copy_str(arg, env_value);

		if (arg == NULL) {
			perror("Reallocation for env-var expansion failed\n");
			exit(EXIT_FAILURE);
		}
	}

	return arg;
}

// parses one single command having into account:
// - the arguments passed to the program
// - stdin/stdout/stderr flow changes
// - environment variables (expand and set)
static struct cmd *
parse_exec(char *buf_cmd)
{
	struct execcmd *c;
	char *tok;
	int idx = 0, argc = 0;

	c = (struct execcmd *) exec_cmd_create(buf_cmd);

	while (buf_cmd[idx] != END_STRING) {
		tok = get_token(buf_cmd, idx);
		idx = idx + strlen(tok);

		if (buf_cmd[idx] != END_STRING)
			idx++;

		if (parse_redir_flow(c, tok))
			continue;

		if (parse_environ_var(c, tok))
			continue;

		tok = expand_environ_var(tok);

		if (tok != NULL && tok[0] == END_STRING) {
			free(tok);
			continue;
		}

		c->argv[argc++] = tok;
	}

	c->argv[argc] = (char *) NULL;
	c->argc = argc;

	return (struct cmd *) c;
}

// parses a command knowing that it contains the '&' char
static struct cmd *
parse_back(char *buf_cmd)
{
	int i = 0;
	struct cmd *e;

	while (buf_cmd[i] != '&')
		i++;

	buf_cmd[i] = END_STRING;

	e = parse_exec(buf_cmd);

	return back_cmd_create(e);
}

// parses a command and checks if it contains the '&'
// (background process) character
static struct cmd *
parse_cmd(char *buf_cmd)
{
	if (strlen(buf_cmd) == 0)
		return NULL;

	int idx;

	// checks if the background symbol is after
	// a redir symbol, in which case
	// it does not have to run in in the 'back'
	if ((idx = block_contains(buf_cmd, '&')) >= 0 && buf_cmd[idx - 1] != '>')
		return parse_back(buf_cmd);

	return parse_exec(buf_cmd);
}

// parses the command line
// looking for the pipe character '|'
struct cmd *
parse_line(char *buf)
{
	struct cmd *r, *l;

	char *right = split_line(buf, '|');

	if (block_contains(right, '|') != -1) {
		r = parse_line(right);  // recursive call
	} else {
		r = parse_cmd(right);
	}

	l = parse_cmd(buf);

	return pipe_cmd_create(l, r);
}
