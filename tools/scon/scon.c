#define SCON_INLINE
#include "../../scon/scon.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char buffer[0x10000];

int main(int argc, char **argv)
{
    int result = 0;
    int isSilent = 0;
    int shouldDump = 0;
    const char* evalExpr = NULL;
    const char* filename = NULL;

    for (int i = 1; i < argc; ++i)
    {
        if (strcmp(argv[i], "-s") == 0 || strcmp(argv[i], "--silent") == 0)
        {
            isSilent = 1;
        }
        else if (strcmp(argv[i], "-e") == 0)
        {
            if (i + 1 < argc)
            {
                evalExpr = argv[++i];
            }
            else
            {
                fprintf(stderr, "error: -e requires an expression argument\n");
                result = 1;
                goto cleanup;
            }
        }
        else if (strcmp(argv[i], "-d") == 0)
        {
            shouldDump = 1;
        }
        else if (argv[i][0] == '-')
        {
            fprintf(stderr, "error: unknown option '%s'\n", argv[i]);
            result = 1;
            goto cleanup;
        }
        else
        {
            filename = argv[i];
        }
    }

    if (filename == NULL && evalExpr == NULL)
    {
        fprintf(stderr, "Usage: %s [options] <filename>\n", argv[0]);
        fprintf(stderr, "Options:\n");
        fprintf(stderr, "  -e <expr>      Evaluate the given expression\n");
        fprintf(stderr, "  -s, --silent   Do not print the evaluation result\n");
        result = 1;
        goto cleanup;
    }

    scon_t* scon = NULL;

    scon_error_t error = SCON_ERROR();
    if (SCON_ERROR_CATCH(&error))
    {
        scon_error_print(&error, stderr);
        result = 1;
        goto cleanup;
    }

    scon = scon_new(&error);

    scon_handle_t ast;
    if (evalExpr != NULL)
    {
        char* exprBuf = malloc(strlen(evalExpr) + 1);
        if (exprBuf != NULL) 
        {
            strcpy(exprBuf, evalExpr);
            ast = scon_parse(scon, exprBuf, strlen(exprBuf), "<eval>");
        }
    }
    else if (filename != NULL)
    {
        ast = scon_parse_file(scon, filename);
    }

    scon_stdlib_register(scon, SCON_STDLIB_ALL);

    scon_function_t* function = scon_compile(scon, &ast);

    if (shouldDump)
    {
        scon_disasm(scon, function, stdout);
    }
    else
    {
        scon_handle_t result = scon_eval(scon, function);
        if (isSilent)
        {
            goto cleanup;
        }

        if (SCON_HANDLE_IS_ITEM(&result))
        {
            scon_item_t* item = SCON_HANDLE_TO_ITEM(&result);
            if (item->type == SCON_ITEM_TYPE_ATOM)
            {
                char* str;
                scon_size_t len;
                scon_handle_get_string(scon, &result, &str, &len);
                printf("%.*s\n", (int)len, str);
                goto cleanup;
            }
        }

        scon_stringify(scon, &result, buffer, sizeof(buffer));
        printf("%s\n", buffer);
    }

cleanup:
    scon_free(scon);

    return result;
}