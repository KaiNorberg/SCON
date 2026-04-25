#ifndef SCON_ERROR_IMPL_H
#define SCON_ERROR_IMPL_H 1

#include "error.h"
#include "eval.h"
#include "item.h"

static inline scon_size_t scon_error_get_region_length(const char* ptr, const char* end)
{
    if (ptr >= end)
    {
        return 0;
    }

    if (*ptr == '(')
    {
        scon_size_t depth = 0;
        scon_bool_t inString = SCON_FALSE;
        const char* current = ptr;
        while (current < end)
        {
            if (*current == '\\' && current + 1 < end)
            {
                current += 2;
                continue;
            }
            if (*current == '"')
            {
                inString = !inString;
            }
            else if (!inString)
            {
                if (*current == '(')
                {
                    depth++;
                }
                else if (*current == ')')
                {
                    depth--;
                    if (depth == 0)
                    {
                        current++;
                        break;
                    }
                }
            }
            current++;
        }
        return (scon_size_t)(current - ptr);
    }
    else if (*ptr == '"')
    {
        const char* current = ptr + 1;
        while (current < end)
        {
            if (*current == '\\' && current + 1 < end)
            {
                current += 2;
                continue;
            }
            if (*current == '"')
            {
                current++;
                break;
            }
            current++;
        }
        return (scon_size_t)(current - ptr);
    }
    else
    {
        const char* current = ptr;
        while (current < end && *current != ' ' && *current != '\t' && *current != '\n' && *current != '\r' &&
            *current != '(' && *current != ')')
        {
            current++;
        }
        return (scon_size_t)(current - ptr);
    }
}

SCON_API void scon_error_print(scon_error_t* error, scon_file_t file)
{
    SCON_ASSERT(error != SCON_NULL);

    scon_size_t row;
    scon_size_t column;
    scon_error_get_row_column(error, &row, &column);

    if (error->path != SCON_NULL)
    {
        SCON_FPRINTF(file, "%s:%zu:%zu: error: %s\n", error->path, row, column, error->message);
    }
    else
    {
        SCON_FPRINTF(file, "error: %s\n", error->message);
    }

    if (error->input != SCON_NULL)
    {
        const char* lineStart = error->input + error->index;
        while (lineStart > error->input && *(lineStart - 1) != '\n')
        {
            lineStart--;
        }

        const char* lineEnd = error->input + error->index;
        while (lineEnd < error->input + error->inputLength && *lineEnd != '\n' && *lineEnd != '\r')
        {
            lineEnd++;
        }

        scon_size_t lineLen = (scon_size_t)(lineEnd - lineStart);
        SCON_FPRINTF(file, " %4zu | %.*s\n", row, (int)lineLen, lineStart);
        SCON_FPRINTF(file, "      | ");

        for (scon_size_t i = 0; i < column - 1; i++)
        {
            SCON_FWRITE(" ", 1, 1, file);
        }
        scon_size_t indicatorLen = error->regionLength > 0 ? error->regionLength : 1;
        for (scon_size_t i = 0; i < indicatorLen; i++)
        {
            SCON_FWRITE("^", 1, 1, file);
        }
        SCON_FWRITE("\n", 1, 1, file);
    }
    else
    {
        SCON_FWRITE("\n", 1, 1, file);
    }
}

SCON_API void scon_error_get_row_column(scon_error_t* error, scon_size_t* row, scon_size_t* column)
{
    SCON_ASSERT(error != SCON_NULL);
    SCON_ASSERT(row != SCON_NULL);
    SCON_ASSERT(column != SCON_NULL);

    *row = 1;
    *column = 1;

    if (error->input == SCON_NULL)
    {
        return;
    }

    for (scon_size_t i = 0; i < error->index; i++)
    {
        if (error->input[i] == '\n')
        {
            (*row)++;
            *column = 1;
        }
        else
        {
            (*column)++;
        }
    }
}

SCON_API void scon_error_set(scon_error_t* error, const char* path, const char* input, scon_size_t inputLength,
    scon_size_t regionLength, scon_size_t position, scon_error_type_t type, const char* message, ...)
{
    SCON_ASSERT(error != SCON_NULL);
    SCON_ASSERT(message != SCON_NULL);

    error->path = path;
    error->input = input;
    error->inputLength = inputLength;
    error->regionLength = regionLength;
    error->index = position;

    scon_va_list args;
    SCON_VA_START(args, message);
    SCON_VSNPRINTF(error->message, SCON_ERROR_MAX_LEN, message, args);
    SCON_VA_END(args);
}

SCON_API void scon_error_get_item_params(struct scon_item* item, const char** path, const char** input,
    scon_size_t* inputLength, scon_size_t* regionLength, scon_size_t* position)
{
    if (item != SCON_NULL && item->input != SCON_NULL)
    {
        *path = item->input->path;
        *input = item->input->buffer;
        *inputLength = item->input->end - item->input->buffer;
        *regionLength = scon_error_get_region_length(item->input->buffer + item->position, item->input->end);
        if (*regionLength == 0)
        {
            *regionLength = 1;
        }

        *position = item->position;
    }
    else
    {
        *path = SCON_NULL;
        *input = SCON_NULL;
        *inputLength = 0;
        *regionLength = 0;
        *position = 0;
    }
}

SCON_API void scon_error_throw_runtime(struct scon* scon, const char* message, ...)
{
    const char* path = SCON_NULL;
    const char* input = SCON_NULL;
    scon_size_t inputLength = 0;
    scon_size_t regionLength = 0;
    scon_size_t position = 0;

    if (scon->evalState != SCON_NULL && scon->evalState->frameCount > 0)
    {
        scon_eval_frame_t* frame = &scon->evalState->frames[scon->evalState->frameCount - 1];
        if (frame->closure != SCON_NULL && frame->closure->function != SCON_NULL)
        {
            scon_function_t* func = frame->closure->function;
            scon_size_t instIndex = frame->ip > func->insts ? (scon_size_t)(frame->ip - func->insts - 1) : 0;
            if (instIndex < func->instCount && func->positions != SCON_NULL)
            {
                position = func->positions[instIndex];
            }

            scon_item_t* funcItem = SCON_CONTAINER_OF(func, scon_item_t, function);
            if (funcItem->input != SCON_NULL)
            {
                path = funcItem->input->path;
                input = funcItem->input->buffer;
                inputLength = (scon_size_t)(funcItem->input->end - funcItem->input->buffer);
                regionLength = scon_error_get_region_length(input + position, funcItem->input->end);
                if (regionLength == 0)
                {
                    regionLength = 1;
                }
            }
        }
    }

    scon_va_list args;
    SCON_VA_START(args, message);
    char formattedMessage[SCON_ERROR_MAX_LEN];
    SCON_VSNPRINTF(formattedMessage, SCON_ERROR_MAX_LEN, message, args);
    SCON_VA_END(args);

    scon_error_set(scon->error, path, input, inputLength, regionLength, position, SCON_ERROR_TYPE_RUNTIME, "%s",
        formattedMessage);
    SCON_LONGJMP(scon->error->jmp, SCON_TRUE);
}

SCON_API void scon_error_check_arity(scon_t* scon, scon_size_t argc, scon_size_t expected, const char* name)
{
    if (SCON_UNLIKELY(argc != expected))
    {
        SCON_ERROR_RUNTIME(scon, "%s expects exactly %zu argument(s), got %zu", name, expected, (scon_size_t)argc);
    }
}

SCON_API void scon_error_check_min_arity(scon_t* scon, scon_size_t argc, scon_size_t min, const char* name)
{
    if (SCON_UNLIKELY(argc < min))
    {
        SCON_ERROR_RUNTIME(scon, "%s expects at least %zu argument(s), got %zu", name, (scon_size_t)min, (scon_size_t)argc);
    }
}

SCON_API void scon_error_check_arity_range(scon_t* scon, scon_size_t argc, scon_size_t min, scon_size_t max,
    const char* name)
{
    if (SCON_UNLIKELY(argc < min || argc > max))
    {
        SCON_ERROR_RUNTIME(scon, "%s expects between %zu and %zu argument(s), got %zu", name, (scon_size_t)min, (scon_size_t)max, (scon_size_t)argc);
    }
}

#endif
