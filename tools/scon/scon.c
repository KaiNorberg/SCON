#define SCON_INLINE
#include "../../scon/scon.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char buffer[0x10000];

int main(int argc, char **argv)
{
    scon_t* scon = scon_new();
    if (scon == NULL) 
    {
        return 1;
    }
    
    int result = 0;
    int shouldStringify = 0;
    int shouldDump = 0;
    const char* evalExpr = NULL;
    const char* filename = NULL;

    for (int i = 1; i < argc; ++i)
    {
        if (strcmp(argv[i], "-s") == 0 || strcmp(argv[i], "--stringify") == 0)
        {
            shouldStringify = 1;
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
        fprintf(stderr, "  -s, --stringify Print the stringified evaluation result\n");
        result = 1;
        goto cleanup;
    }

    if (SCON_CATCH(scon))
    {
        printf("%s\n", scon_error_get(scon));
        result = 1;
        goto cleanup;
    }

    scon_handle_t ast;
    if (evalExpr != NULL)
    {
        ast = scon_parse(scon, evalExpr, strlen(evalExpr), "<eval>");
    }
    else if (filename != NULL)
    {
        ast = scon_parse_file(scon, filename);
    }

    scon_builtin_register(scon, SCON_BUILTIN_ALL);

    scon_function_t* function = scon_compile(scon, &ast);

    if (shouldDump)
    {
        scon_disasm(scon, function, stdout);
    }

    /*if (shouldStringify)
    {
        scon_stringify(scon, buffer, sizeof(buffer));
        printf("%s\n", buffer);
    }*/

cleanup:
    scon_free(scon);

    return result;
}