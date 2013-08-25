/*
* Copyright (c) 2012, Alexander I. Mykyta
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*
* 1. Redistributions of source code must retain the above copyright notice, this
*    list of conditions and the following disclaimer.
* 2. Redistributions in binary form must reproduce the above copyright notice,
*    this list of conditions and the following disclaimer in the documentation
*    and/or other materials provided with the distribution.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
* ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
* ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
* ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
* SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/*==================================================================================================
* File History:
* NAME          DATE         COMMENTS
* Alex M.       2011-01-04   born
* Alex M.       2013-07-26   Overhauled code
*
*=================================================================================================*/

/**
* \addtogroup MOD_CLI
* \{
**/

/**
* \file
* \brief Code for \ref MOD_CLI "Command Line Interface"
* \author Alex Mykyta
**/

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#include "cli.h"
#include <cli_commands.h>

///\todo figure out a nice gcc attribute to force this into flash. "const" isn't technically correct
static const cmdentry_t CommandTable[] = {CMDTABLE};

static bool echo = true;

void cli_print_prompt(void);
void cli_print_error(int error);
void cli_print_notfound(char *strcmd);

#define CMDCOUNT    (sizeof(CommandTable)/sizeof(cmdentry_t))

//--------------------------------------------------------------------------------------------------
static uint16_t split_args(char *str, char *argv[])
{

    enum states { SEEK, IN_WORD, IN_QUOTES } state;
    char c;
    uint16_t count;

    count = 0;
    state = SEEK;

    while (*str)
    {
        c = *str;

        switch (state)
        {
        case SEEK:
            if (c == ' ') break;

            if (c == '"')
            {
                state = IN_QUOTES;
                argv[count] = str + 1;
                break;
            }

            state = IN_WORD;
            argv[count] = str;
            break;
        case IN_WORD:
            if (c == ' ')
            {
                *str = 0;
                state = SEEK;
                count++;
                if (count == CLI_MAX_ARGC) return(count);
                break;
            }
            break;
        case IN_QUOTES:
            if (c == '"')
            {
                *str = 0;
                state = SEEK;
                count++;
                if (count == CLI_MAX_ARGC) return(count);
                break;
            }
            break;
        }
        str++;
    }

    if (state != SEEK)
    {
        count++;
    }

    return(count);
}

//--------------------------------------------------------------------------------------------------
static int compare_cmdentry(const void *a, const void *b)
{
    const cmdentry_t *cmd_a = a;
    const cmdentry_t *cmd_b = b;
    return(strcmp(cmd_a->strCommand, cmd_b->strCommand));
}

//--------------------------------------------------------------------------------------------------
void cli_process_char(char inchar)
{
    static char strin[CLI_STRBUF_SIZE];
    static uint16_t stridx = 0;

    if (inchar == '\r')   // recvd return
    {
        // Process Command
        strin[stridx] = 0; // null terminate

        if (stridx != 0)
        {
            cli_puts("\r\n");

            // Split the string into argv table
            char *argv[CLI_MAX_ARGC];
            uint16_t argc;

            argc = split_args(strin, argv);

            if (argc > 0)
            {
                // Use binary search to lookup the command
                cmdentry_t *command;
                cmdentry_t key;
                key.strCommand = argv[0];
                command = bsearch(&key, CommandTable, CMDCOUNT, sizeof(cmdentry_t), compare_cmdentry);

                if (command)
                {
                    int err;
                    err = (command->cmdptr)(argc, argv); // Execute command

                    if (err)
                    {
                        cli_print_error(err);
                    }
                }
                else
                {
                    cli_print_notfound(argv[0]);
                }
            }
        }

        cli_print_prompt();

        stridx = 0;
    }
    else if ((inchar == 0x7F) || (inchar == '\b'))   // Backspace Key
    {
        if (stridx != 0)
        {
            stridx--;
            cli_puts("\b \b");
        }
    }
    else if (inchar == '\n')
    {
        // discard line feed characters.
    }
    else
    {
        if (stridx < CLI_STRBUF_SIZE)
        {
            strin[stridx++] = inchar;
            if (echo) cli_putc(inchar);
        }
    }
}

//--------------------------------------------------------------------------------------------------
void cli_echo_off(void)
{
    echo = false;
}

void cli_echo_on(void)
{
    echo = true;
}

///\}
