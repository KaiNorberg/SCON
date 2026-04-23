#include "defs.h"
#ifndef SCON_STDLIB_SYSTEM_IMPL_H
#define SCON_STDLIB_SYSTEM_IMPL_H 1

#include "atom.h"
#include "compile.h"
#include "core.h"
#include "eval.h"
#include "handle.h"
#include "parse.h"
#include "stdlib_system.h"
#include "stringify.h"

/** **`(eval <item>) -> <item>`**

Evaluates the provided item and returns the result.

**`(parse <atom>) -> <expression>`**

Parses the provided string into a SCON expression without evaluating it. */

static void scon_resolve_path(scon_t* scon, const char* path, scon_size_t pathLen, char* outPath, scon_size_t maxLen)
{
    scon_bool_t isAbsolute = SCON_FALSE;
    if (pathLen > 0 && (path[0] == '/' || path[0] == '\\'))
    {
        isAbsolute = SCON_TRUE;
    }
    if (pathLen > 1 && ((path[0] >= 'a' && path[0] <= 'z') || (path[0] >= 'A' && path[0] <= 'Z')) && path[1] == ':')
    {
        isAbsolute = SCON_TRUE;
    }

    if (isAbsolute || scon->evalState == SCON_NULL || scon->evalState->frameCount == 0)
    {
        if (pathLen >= maxLen)
        {
            SCON_ERROR_RUNTIME(scon, "path too long");
        }
        SCON_MEMCPY(outPath, path, pathLen);
        outPath[pathLen] = '\0';
        return;
    }

    scon_input_t* input = SCON_NULL;
    for (scon_size_t i = scon->evalState->frameCount; i > 0; i--)
    {
        scon_eval_frame_t* frame = &scon->evalState->frames[i - 1];
        scon_item_t* funcItem = SCON_CONTAINER_OF(frame->closure->function, scon_item_t, function);
        if (funcItem->input != SCON_NULL && funcItem->input->path[0] != '\0')
        {
            input = funcItem->input;
            break;
        }
    }

    if (input == SCON_NULL)
    {
        if (pathLen >= maxLen)
        {
            SCON_ERROR_RUNTIME(scon, "path too long");
        }
        SCON_MEMCPY(outPath, path, pathLen);
        outPath[pathLen] = '\0';
        return;
    }

    const char* lastSlash = SCON_NULL;
    for (const char* p = input->path; *p != '\0'; p++)
    {
        if (*p == '/' || *p == '\\')
        {
            lastSlash = p;
        }
    }

    if (lastSlash == SCON_NULL)
    {
        if (pathLen >= maxLen)
        {
            SCON_ERROR_RUNTIME(scon, "path too long");
        }
        SCON_MEMCPY(outPath, path, pathLen);
        outPath[pathLen] = '\0';
        return;
    }

    scon_size_t dirLen = (scon_size_t)(lastSlash - input->path) + 1;
    if (dirLen + pathLen >= maxLen)
    {
        SCON_ERROR_RUNTIME(scon, "path too long");
    }

    SCON_MEMCPY(outPath, input->path, dirLen);
    SCON_MEMCPY(outPath + dirLen, path, pathLen);
    outPath[dirLen + pathLen] = '\0';
}

SCON_API scon_handle_t scon_load(struct scon* scon, scon_handle_t* path)
{
    SCON_ASSERT(scon != SCON_NULL);

    char* pathStr;
    scon_size_t pathLen;
    scon_handle_get_string_params(scon, path, &pathStr, &pathLen);

    char pathBuf[SCON_PATH_MAX];
    scon_resolve_path(scon, pathStr, pathLen, pathBuf, SCON_PATH_MAX);

    scon_handle_t ast = scon_parse_file(scon, pathBuf);

    scon_function_t* function = scon_compile(scon, &ast);

    scon_handle_t result = scon_eval(scon, function);
    return result;
}

SCON_API scon_handle_t scon_read_file(struct scon* scon, scon_handle_t* path)
{
    SCON_ASSERT(scon != SCON_NULL);

    char* pathStr;
    scon_size_t pathLen;
    scon_handle_get_string_params(scon, path, &pathStr, &pathLen);

    char pathBuf[SCON_PATH_MAX];
    scon_resolve_path(scon, pathStr, pathLen, pathBuf, SCON_PATH_MAX);

    scon_file_t file = SCON_FOPEN(pathBuf, "rb");
    if (file == SCON_NULL)
    {
        SCON_ERROR_RUNTIME(scon, "could not open file '%s'", pathBuf);
    }

    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char* buffer = (char*)SCON_MALLOC(size);
    if (buffer == SCON_NULL)
    {
        SCON_FCLOSE(file);
        SCON_ERROR_INTERNAL(scon, "out of memory");
    }

    if (SCON_FREAD(buffer, 1, size, file) != (scon_size_t)size)
    {
        SCON_FREE(buffer);
        SCON_FCLOSE(file);
        SCON_ERROR_RUNTIME(scon, "could not read file '%s'", pathBuf);
    }

    SCON_FCLOSE(file);

    scon_handle_t result = SCON_HANDLE_FROM_ATOM(scon_atom_lookup(scon, buffer, size, SCON_ATOM_LOOKUP_QUOTED));
    SCON_FREE(buffer);

    return result;
}

SCON_API scon_handle_t scon_write_file(struct scon* scon, scon_handle_t* path, scon_handle_t* content)
{
    SCON_ASSERT(scon != SCON_NULL);

    char* pathStr;
    scon_size_t pathLen;
    scon_handle_get_string_params(scon, path, &pathStr, &pathLen);

    char pathBuf[SCON_PATH_MAX];
    scon_resolve_path(scon, pathStr, pathLen, pathBuf, SCON_PATH_MAX);

    char* contentStr;
    scon_size_t contentLen;
    scon_handle_get_string_params(scon, content, &contentStr, &contentLen);

    scon_file_t file = SCON_FOPEN(pathBuf, "wb");
    if (file == SCON_NULL)
    {
        SCON_ERROR_RUNTIME(scon, "could not open file '%s' for writing", pathBuf);
    }

    if (SCON_FWRITE(contentStr, 1, contentLen, file) != contentLen)
    {
        SCON_FCLOSE(file);
        SCON_ERROR_RUNTIME(scon, "could not write to file '%s'", pathBuf);
    }

    SCON_FCLOSE(file);
    return *content;
}

SCON_API scon_handle_t scon_read_char(struct scon* scon)
{
    SCON_ASSERT(scon != SCON_NULL);

    int c = SCON_FGETC(SCON_STDIN);
    if (c == EOF)
    {
        return scon_handle_nil(scon);
    }

    char ch = (char)c;
    return SCON_HANDLE_FROM_ATOM(scon_atom_lookup(scon, &ch, 1, SCON_ATOM_LOOKUP_NONE));
}

SCON_API scon_handle_t scon_read_line(struct scon* scon)
{
    SCON_ASSERT(scon != SCON_NULL);

    char stackBuffer[SCON_STACK_BUFFER_SIZE];
    scon_size_t capacity = SCON_STACK_BUFFER_SIZE;
    char* buffer = stackBuffer;
    scon_size_t length = 0;

    while (SCON_TRUE)
    {
        int c = SCON_FGETC(SCON_STDIN);
        if (c == EOF || c == '\n')
        {
            if (c == EOF && length == 0)
            {
                if (buffer != stackBuffer)
                {
                    SCON_FREE(buffer);
                }
                return scon_handle_nil(scon);
            }
            break;
        }

        if (length + 1 >= capacity)
        {
            scon_size_t newCapacity = capacity * 2;
            char* newBuffer = (char*)SCON_MALLOC(newCapacity);
            if (newBuffer == SCON_NULL)
            {
                if (buffer != stackBuffer)
                {
                    SCON_FREE(buffer);
                }
                SCON_ERROR_INTERNAL(scon, "out of memory");
            }
            SCON_MEMCPY(newBuffer, buffer, length);
            if (buffer != stackBuffer)
            {
                SCON_FREE(buffer);
            }
            buffer = newBuffer;
            capacity = newCapacity;
        }
        buffer[length++] = (char)c;
    }

    scon_handle_t result = SCON_HANDLE_FROM_ATOM(scon_atom_lookup(scon, buffer, length, SCON_ATOM_LOOKUP_QUOTED));
    if (buffer != stackBuffer)
    {
        SCON_FREE(buffer);
    }
    return result;
}

SCON_API scon_handle_t scon_print(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    SCON_ASSERT(scon != SCON_NULL);

    for (scon_size_t i = 0; i < argc; i++)
    {
        char* str;
        scon_size_t len;
        scon_handle_get_string_params(scon, &argv[i], &str, &len);
        SCON_FWRITE(str, 1, len, SCON_STDOUT);
        if (i < argc - 1)
        {
            SCON_FWRITE(" ", 1, 1, SCON_STDOUT);
        }
    }
    return scon_handle_nil(scon);
}

SCON_API scon_handle_t scon_println(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    SCON_ASSERT(scon != SCON_NULL);

    scon_handle_t handle = scon_print(scon, argc, argv);
    SCON_FWRITE("\n", 1, 1, SCON_STDOUT);
    return handle;
}

SCON_API scon_handle_t scon_ord(struct scon* scon, scon_handle_t* handle)
{
    SCON_ASSERT(scon != SCON_NULL);
    SCON_ASSERT(handle != SCON_NULL);

    char* str;
    scon_size_t len;
    scon_handle_get_string_params(scon, handle, &str, &len);

    if (len == 0)
    {
        SCON_ERROR_RUNTIME(scon, "ord expects a non-empty atom");
    }

    return SCON_HANDLE_FROM_INT((scon_int64_t)(scon_uint8_t)str[0]);
}

SCON_API scon_handle_t scon_chr(struct scon* scon, scon_handle_t* handle)
{
    SCON_ASSERT(scon != SCON_NULL);
    SCON_ASSERT(handle != SCON_NULL);

    scon_handle_t h = *handle;
    if (!SCON_HANDLE_IS_INT(&h))
    {
        scon_handle_ensure_item(scon, &h);
        scon_item_t* item = SCON_HANDLE_TO_ITEM(&h);
        if (!(SCON_HANDLE_GET_FLAGS(&h) & SCON_ITEM_FLAG_INT_SHAPED))
        {
            SCON_ERROR_RUNTIME(scon, "chr expects an integer, got %s", scon_item_type_str(item->type));
        }
    }

    scon_int64_t val = SCON_HANDLE_TO_INT(&h);
    if (val < 0 || val > 255)
    {
        SCON_ERROR_RUNTIME(scon, "chr expects an integer in range 0-255, got %lld", (long long)val);
    }

    char c = (char)(scon_uint8_t)val;
    return SCON_HANDLE_FROM_ATOM(scon_atom_lookup(scon, &c, 1, SCON_ATOM_LOOKUP_NONE));
}

SCON_API scon_handle_t scon_format(scon_t* scon, scon_size_t argc, scon_handle_t* argv)
{
    SCON_ASSERT(scon != SCON_NULL);

    if (argc == 0)
    {
        return SCON_HANDLE_FROM_ATOM(scon_atom_lookup(scon, "", 0, SCON_ATOM_LOOKUP_NONE));
    }

    char* fmtStr;
    scon_size_t fmtLen;
    scon_handle_get_string_params(scon, &argv[0], &fmtStr, &fmtLen);

    scon_size_t totalLen = 0;
    scon_size_t argIndex = 1;

    for (scon_size_t i = 0; i < fmtLen; i++)
    {
        if (fmtStr[i] == '{')
        {
            if (i + 1 < fmtLen && fmtStr[i + 1] == '{')
            {
                totalLen++;
                i++;
                continue;
            }

            scon_size_t j = i + 1;
            scon_int64_t explicitIndex = -1;
            if (j < fmtLen && fmtStr[j] >= '0' && fmtStr[j] <= '9')
            {
                explicitIndex = 0;
                while (j < fmtLen && fmtStr[j] >= '0' && fmtStr[j] <= '9')
                {
                    explicitIndex = explicitIndex * 10 + (fmtStr[j] - '0');
                    j++;
                }
            }

            if (j < fmtLen && fmtStr[j] == '}')
            {
                scon_size_t idx = (explicitIndex != -1) ? (scon_size_t)explicitIndex + 1 : argIndex++;
                if (idx >= argc)
                {
                    SCON_ERROR_RUNTIME(scon, "format index out of range");
                }
                totalLen += scon_stringify(scon, &argv[idx], SCON_NULL, 0);
                i = j;
                continue;
            }

            SCON_ERROR_RUNTIME(scon, "invalid format string");
        }
        else if (fmtStr[i] == '}')
        {
            if (i + 1 < fmtLen && fmtStr[i + 1] == '}')
            {
                totalLen++;
                i++;
                continue;
            }

            SCON_ERROR_RUNTIME(scon, "single '}' encountered in format string");
        }
        totalLen++;
    }

    char stackBuffer[SCON_STACK_BUFFER_SIZE];
    char* buffer = (totalLen + 1 <= SCON_STACK_BUFFER_SIZE) ? stackBuffer : (char*)SCON_MALLOC(totalLen + 1);
    if (buffer == SCON_NULL)
    {
        SCON_ERROR_INTERNAL(scon, "out of memory");
    }

    scon_size_t currentPos = 0;
    argIndex = 1;

    for (scon_size_t i = 0; i < fmtLen; i++)
    {
        if (fmtStr[i] == '{')
        {
            if (i + 1 < fmtLen && fmtStr[i + 1] == '{')
            {
                buffer[currentPos++] = '{';
                i++;
                continue;
            }

            scon_size_t j = i + 1;
            scon_int64_t explicitIndex = -1;
            if (j < fmtLen && fmtStr[j] >= '0' && fmtStr[j] <= '9')
            {
                explicitIndex = 0;
                while (j < fmtLen && fmtStr[j] >= '0' && fmtStr[j] <= '9')
                {
                    explicitIndex = explicitIndex * 10 + (fmtStr[j] - '0');
                    j++;
                }
            }

            if (j < fmtLen && fmtStr[j] == '}')
            {
                scon_size_t idx = (explicitIndex != -1) ? (scon_size_t)explicitIndex + 1 : argIndex++;
                currentPos += scon_stringify(scon, &argv[idx], buffer + currentPos, totalLen - currentPos + 1);
                i = j;
                continue;
            }
        }
        else if (fmtStr[i] == '}')
        {
            if (i + 1 < fmtLen && fmtStr[i + 1] == '}')
            {
                buffer[currentPos++] = '}';
                i++;
                continue;
            }
        }
        buffer[currentPos++] = fmtStr[i];
    }

    scon_handle_t result = SCON_HANDLE_FROM_ATOM(scon_atom_lookup(scon, buffer, totalLen, SCON_ATOM_LOOKUP_NONE));
    if (buffer != stackBuffer)
    {
        SCON_FREE(buffer);
    }
    return result;
}

SCON_API scon_handle_t scon_now(scon_t* scon)
{
    SCON_ASSERT(scon != SCON_NULL);

    return SCON_HANDLE_FROM_FLOAT((scon_float_t)SCON_TIME());
}

SCON_API scon_handle_t scon_uptime(scon_t* scon)
{
    SCON_ASSERT(scon != SCON_NULL);

    return SCON_HANDLE_FROM_FLOAT((scon_float_t)SCON_CLOCK());
}

/** **`(env!) -> <list>`**

Returns all environment variables as association sub-lists.

**`(args!) -> <list>`**

Returns a list of all command line arguments. */

SCON_API scon_handle_t scon_env(struct scon* scon)
{
    SCON_ASSERT(scon != SCON_NULL);

    extern char** environ;
    scon_size_t count = 0;
    while (environ[count] != SCON_NULL)
    {
        count++;
    }

    scon_list_t* list = scon_list_new(scon, count);
    for (scon_size_t i = 0; i < count; i++)
    {
        char* env = environ[i];
        char* eq = strchr(env, '=');
        if (eq != SCON_NULL)
        {
            scon_list_t* pair = scon_list_new(scon, 2);
            scon_list_append(scon, pair, SCON_HANDLE_FROM_ATOM(scon_atom_lookup(scon, env, (scon_size_t)(eq - env), SCON_ATOM_LOOKUP_NONE)));
            scon_list_append(scon, pair, SCON_HANDLE_FROM_ATOM(scon_atom_lookup(scon, eq + 1, SCON_STRLEN(eq + 1), SCON_ATOM_LOOKUP_QUOTED)));
            scon_list_append(scon, list, SCON_HANDLE_FROM_LIST(pair));
        }
    }

    return SCON_HANDLE_FROM_LIST(list);
}

SCON_API scon_handle_t scon_args(struct scon* scon)
{
    SCON_ASSERT(scon != SCON_NULL);

    if (scon->argc == 0)
    {
        return scon_handle_nil(scon);
    }

    scon_list_t* list = scon_list_new(scon, scon->argc);
    for (scon_size_t i = 0; i < scon->argc; i++)
    {
        scon_list_append(scon, list, SCON_HANDLE_FROM_ATOM(scon_atom_lookup(scon, scon->argv[i], SCON_STRLEN(scon->argv[i]), SCON_ATOM_LOOKUP_QUOTED)));
    }

    return SCON_HANDLE_FROM_LIST(list);
}

#endif