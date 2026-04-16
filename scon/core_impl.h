#ifndef SCON_CORE_IMPL_H
#define SCON_CORE_IMPL_H 1

#include "core_api.h"
#include "core_internal.h"

static inline _scon_input_t* scon_input_new(scon_t* scon, const char* buffer, scon_size_t length, const char* path)
{
    _scon_input_t* input;
    if (scon->input == SCON_NULL)
    {
        input = &scon->firstInput;
    }
    else
    {
        input = SCON_MALLOC(sizeof(_scon_input_t));
        if (input == SCON_NULL)
        {
            SCON_THROW(scon, "out of memory");
        }
    }
    input->prev = scon->input;
    input->buffer = buffer;
    input->length = length;
    SCON_STRNCPY(input->path, path, SCON_PATH_MAX - 1);
    input->path[SCON_PATH_MAX - 1] = '\0';
    scon->input = input;
    return input;
}

SCON_API scon_t* scon_new(void)
{
    scon_t* scon = SCON_CALLOC(1, sizeof(scon_t));
    if (scon == SCON_NULL)
    {
        return SCON_NULL;
    }

    scon->gcThreshold = _SCON_GC_THRESHOLD_INITIAL;
    _scon_list_init(scon, &scon->retained);

    return scon;
}

SCON_API void scon_free(scon_t* scon)
{
    if (scon == SCON_NULL)
    {
        return;
    }

    _scon_list_deinit(scon, &scon->retained);

    _scon_node_block_t* block = scon->block;
    while (block != SCON_NULL)
    {
        _scon_node_block_t* next = block->next;
        for (int i = 0; i < _SCON_ITEM_BLOCK_MAX; i++)
        {
            _scon_node_t* node = &block->items[i];
            _scon_node_free(scon, node);
        }
        
        if (block != &scon->firstBlock) {
            SCON_FREE(block);
        }
        block = next;
    }

    _scon_input_t* input = scon->input;
    while (input != SCON_NULL)
    {
        _scon_input_t* next = input->prev;
        SCON_FREE((void*)input->buffer);
        if (input != &scon->firstInput) {
            SCON_FREE(input);
        }
        input = next;
    }

    SCON_FREE(scon);
}

static void _scon_error_set_char(char* buf, scon_size_t* len, scon_size_t max, char c)
{
    if (*len < max - 1)
    {
        buf[(*len)++] = c;
    }
}

static void _scon_error_set_str(char* buf, scon_size_t* len, scon_size_t max, const char* str)
{
    while (*str && *len < max - 1)
    {
        buf[(*len)++] = *str++;
    }
}

static scon_size_t _scon_error_set_uint(char* buf, scon_size_t* len, scon_size_t max, scon_size_t val,
    scon_size_t minWidth)
{
    char num[32];
    scon_size_t numLen = 0;
    if (val == 0)
    {
        num[numLen++] = '0';
    }
    while (val > 0)
    {
        num[numLen++] = (char)('0' + (val % 10));
        val /= 10;
    }
    scon_size_t digits = numLen;
    while (minWidth > digits)
    {
        _scon_error_set_char(buf, len, max, ' ');
        minWidth--;
    }
    while (numLen > 0)
    {
        _scon_error_set_char(buf, len, max, num[--numLen]);
    }
    return digits;
}

SCON_API void scon_error_set(scon_t* scon, const char* message, const char* input, scon_size_t length, scon_size_t position)
{
    if (position == (scon_size_t)-1 || position > length)
    {
        char* buf = scon->error;
        scon_size_t max = _SCON_ERROR_MAX_LEN;
        scon_size_t len = 0;

        _scon_error_set_str(buf, &len, max, "scon: error: ");
        _scon_error_set_str(buf, &len, max, message);

        buf[len] = '\0';
        return;
    }

    scon_size_t line = 1;
    scon_size_t column = 1;
    scon_size_t lineStart = 0;
    for (scon_size_t i = 0; i < position; i++)
    {
        if (input[i] == '\n')
        {
            line++;
            column = 1;
            lineStart = i + 1;
        }
        else
        {
            column++;
        }
    }

    char* buf = scon->error;
    scon_size_t max = _SCON_ERROR_MAX_LEN;
    scon_size_t len = 0;

    _scon_error_set_str(buf, &len, max, "scon:");

    _scon_error_set_uint(buf, &len, max, line, 0);

    _scon_error_set_char(buf, &len, max, ':');

    _scon_error_set_uint(buf, &len, max, column, 0);

    _scon_error_set_str(buf, &len, max, ": error: ");

    _scon_error_set_str(buf, &len, max, message);

    _scon_error_set_str(buf, &len, max, "\n  | ");

    scon_size_t lineNumLen = _scon_error_set_uint(buf, &len, max, line, 4);

    _scon_error_set_str(buf, &len, max, " | ");

    scon_size_t i = lineStart;
    while (i < length && input[i] != '\n')
    {
        if (input[i] != '\r')
        {
            _scon_error_set_char(buf, &len, max, input[i]);
        }
        i++;
    }

    _scon_error_set_str(buf, &len, max, "\n  | ");
    scon_size_t indicatorPad;
    if (lineNumLen < 4)
    {
        indicatorPad = 4;
    }
    else
    {
        indicatorPad = lineNumLen;
    }
    indicatorPad += 3;
    while (indicatorPad-- > 0)
    {
        _scon_error_set_char(buf, &len, max, ' ');
    }
    for (scon_size_t j = lineStart; j < position; j++)
    {
        if (input[j] == '\t')
        {
            _scon_error_set_char(buf, &len, max, '\t');
        }
        else if (input[j] != '\r')
        {
            _scon_error_set_char(buf, &len, max, ' ');
        }
    }
    _scon_error_set_char(buf, &len, max, '^');
    _scon_error_set_char(buf, &len, max, ' ');

    buf[len] = '\0';
}

SCON_API const char* scon_error_get(scon_t* scon)
{
    return scon->error;
}

SCON_API scon_jmp_buf_t* scon_jmp_buf_get(scon_t* scon)
{
    return &scon->jmp;
}

SCON_API const char* scon_input_get(scon_t* scon)
{
    return scon->input ? scon->input->buffer : SCON_NULL;
}

SCON_API scon_size_t scon_input_get_length(scon_t* scon)
{
    return scon->input ? scon->input->length : 0;
}

SCON_API const char* scon_input_get_path(scon_t* scon)
{
    return scon->input ? scon->input->path : SCON_NULL;
}

#endif
