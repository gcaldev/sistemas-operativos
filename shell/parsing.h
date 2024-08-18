#ifndef PARSING_H
#define PARSING_H

#include "defs.h"
#include "types.h"
#include "createcmd.h"
#include "utils.h"

// Declare runcmd global variable
extern int status;

struct cmd *parse_line(char *b);
extern int status;

#endif  // PARSING_H
