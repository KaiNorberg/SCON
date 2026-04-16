#ifndef SCON_CORE_INTERNAL_H
#define SCON_CORE_INTERNAL_H 1

struct _scon_node;

#include "core_api.h"
#include "item_internal.h"
#include "atom_internal.h"
#include "list_internal.h"

#define _SCON_ERROR_MAX_LEN 512
#define _SCON_BUCKETS_MAX 512

#define _SCON_GC_THRESHOLD_INITIAL 1024

typedef struct _scon_input
{
    struct _scon_input* prev;
    const char* buffer;
    scon_size_t length;
    char path[SCON_PATH_MAX];
} _scon_input_t;

struct scon
{
    /*scon_callbacks_t cb;
    _scon_input_t* input;
    _scon_node_block_t* itemBlocks;
    scon_size_t allocations;
    scon_size_t gcThreshold;
    scon_item_t* root;
    scon_item_t* freeList;
    scon_item_t* listNil;
    scon_item_t* atomTrue;
    scon_item_t* atomFalse;*/
    scon_size_t blocksAllocated;
    scon_size_t gcThreshold;
    _scon_list_t retained;
    _scon_node_block_t* block;
    struct _scon_node* freeList;
    _scon_input_t* input;
    scon_jmp_buf_t jmp;
    _scon_node_block_t firstBlock;
    _scon_input_t firstInput;
    struct _scon_atom* atomBuckets[_SCON_BUCKETS_MAX];
    char error[_SCON_ERROR_MAX_LEN];
};

static inline _scon_input_t* scon_input_new(scon_t* scon, const char* buffer, scon_size_t length, const char* path);

#endif
