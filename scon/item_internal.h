#ifndef SCON_ITEM_INTERNAL_H
#define SCON_ITEM_INTERNAL_H 1

#include "core_api.h"
#include "item_api.h"
#include "atom_internal.h"
#include "list_internal.h"

struct _scon_input;

typedef scon_uint16_t _scon_node_flags_t;
#define _SCON_NODE_FLAG_NONE 0
#define _SCON_NODE_FLAG_FALSY (1 << 0)
#define _SCON_NODE_FLAG_INT_SHAPED (1 << 1)
#define _SCON_NODE_FLAG_FLOAT_SHAPED (1 << 2)
#define _SCON_NODE_FLAG_QUOTED (1 << 3)
#define _SCON_NODE_FLAG_GC_MARK (1 << 4)

typedef struct _scon_node _scon_node_t;

struct _scon_node
{
    struct _scon_input* input;
    scon_uint32_t position;
    scon_item_type_t type;
    _scon_node_flags_t flags;
    union
    {
        scon_uint32_t length;
        _scon_atom_t atom;
        _scon_list_t list;
        _scon_node_t* free;
    };
};

#define _SCON_ITEM_BLOCK_MAX 1024

typedef struct _scon_node_block
{
    struct _scon_node_block* next;
    _scon_node_t items[_SCON_ITEM_BLOCK_MAX];
} _scon_node_block_t;

#ifdef SCON_USE_NAN_BOXING

#ifdef _Static_assert
_Static_assert(sizeof(_scon_node_t) == 64, "scon_item_t must be 64 bytes");
#endif

#define _SCON_ITEM_TAG_INT  0xFFFC000000000000ULL
#define _SCON_ITEM_TAG_NODE  0xFFFE000000000000ULL
#define _SCON_ITEM_TAG_MASK 0xFFFF000000000000ULL
#define _SCON_ITEM_VAL_MASK 0x0000FFFFFFFFFFFFULL

#define _SCON_ITEM_FROM_INT(_val) \
    (_SCON_ITEM_TAG_INT | ((scon_item_t)(scon_uint32_t)(_val)))
#define _SCON_ITEM_FROM_FLOAT(_val) \
    (((union { double d; scon_item_t u; }){ .d = (_val) }).u)
#define _SCON_ITEM_FROM_NODE(_ptr) \
    (_SCON_ITEM_TAG_NODE | ((scon_item_t)(void*)(_ptr) & _SCON_ITEM_VAL_MASK))

#define _SCON_ITEM_IS_INT(_item) \
    ((*(_item) & _SCON_ITEM_TAG_MASK) == _SCON_ITEM_TAG_INT)
#define _SCON_ITEM_IS_FLOAT(_item) \
    (*(_item) < _SCON_ITEM_TAG_INT)
#define _SCON_ITEM_IS_NODE(_item) \
    ((*(_item) & _SCON_ITEM_TAG_MASK) == _SCON_ITEM_TAG_NODE)

#define _SCON_ITEM_TO_INT(_item) \
    ((scon_int32_t)*(_item))
#define _SCON_ITEM_TO_FLOAT(_item) \
    (((union { scon_item_t u; double d; }){ .u = *(_item) }).d)
#define _SCON_ITEM_TO_NODE(_item) \
    ((_scon_node_t*)(void*)(*(_item) & _SCON_ITEM_VAL_MASK))

#else

#define _SCON_ITEM_FROM_INT(_val) \
    ((scon_item_t){ .internalType = _SCON_INTERNAL_TYPE_INT, .as.i = (scon_int64_t)(_val) })
#define _SCON_ITEM_FROM_FLOAT(_val) \
    ((scon_item_t){ .internalType = _SCON_INTERNAL_TYPE_FLOAT, .as.f = (scon_float_t)(_val) })
#define _SCON_ITEM_FROM_NODE(_ptr) \
    ((scon_item_t){ .internalType = _SCON_INTERNAL_TYPE_NODE, .as.node = (void*)(_ptr) })

#define _SCON_ITEM_IS_INT(_item) \
    ((_item)->internalType == _SCON_INTERNAL_TYPE_INT)
#define _SCON_ITEM_IS_FLOAT(_item) \
    ((_item)->internalType == _SCON_INTERNAL_TYPE_FLOAT)
#define _SCON_ITEM_IS_NODE(_item) \
    ((_item)->internalType == _SCON_INTERNAL_TYPE_NODE)

#define _SCON_ITEM_TO_INT(_item) \
    ((_item)->as.i)
#define _SCON_ITEM_TO_FLOAT(_item) \
    ((_item)->as.f)
#define _SCON_ITEM_TO_NODE(_item) \
    ((_scon_node_t*)((_item)->as.node))

#endif

static inline _scon_node_t* _scon_node_new(scon_t* scon);

static inline void _scon_node_free(scon_t* scon, _scon_node_t* item);

static inline void _scon_item_error_set(scon_t* scon, _scon_node_t* item, const char* message);

static inline void _scon_item_throw(scon_t* scon, _scon_node_t* item, const char* message);

#endif
