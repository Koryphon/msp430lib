#ifndef _CLI_COMMANDS_H
#define _CLI_COMMANDS_H

#include <cli.h>
#include <stdint.h>

// maximum length of a command input line
#define CLI_STRBUF_SIZE    64

// Maximum number of arguments in a command (including command).
#define CLI_MAX_ARGC    5

// Table of commands: {"command_word" , function_name }
// Command words MUST be in alphabetical (ascii) order!! (A-Z then a-z)
#define CMDTABLE    {"bye"      , cmdBye      },\
                    {"hi"       , cmdHello    },\
                    {"listargs" , cmdArgList  }

// Custom command function prototypes:
int cmdArgList(uint16_t argc, char *argv[]);
int cmdBye(uint16_t argc, char *argv[]);
int cmdHello(uint16_t argc, char *argv[]);

#endif
