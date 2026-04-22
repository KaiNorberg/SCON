#ifndef SCON_ERROR_IMPL_H
#define SCON_ERROR_IMPL_H 1

#include "error.h"
#include "eval.h"
#include "item.h"

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
        switch (item->type)
        {
        case SCON_ITEM_TYPE_ATOM:
            *regionLength = item->atom.length;
            break;
        case SCON_ITEM_TYPE_LIST:
        {
            scon_size_t depth = 1;
            const char* ptr = item->input->buffer + item->position;
            const char* start = ptr;
            while (ptr < item->input->end)
            {
                if (*ptr == '\\')
                {
                    ptr++;
                }
                if (*ptr == '(')
                {
                    depth++;
                }
                else if (*ptr == ')')
                {
                    depth--;
                    if (depth == 0)
                    {
                        break;
                    }
                }
                ptr++;
            }
            *regionLength = (scon_size_t)(ptr - start);
        }
        break;
        default:
            *regionLength = 1;
            break;
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

#endif
