#ifndef SCON_LIST_INTERNAL_H
#define SCON_LIST_INTERNAL_H 1

#include "core_api.h"
#include "item_api.h"

struct _scon_node;

#define _SCON_LIST_SHORT_MAX 5
#define _SCON_LIST_GROWTH_FACTOR 2

typedef struct _scon_list
{
    scon_uint32_t length;
    scon_uint32_t capacity;
    union {
        scon_item_t shortItems[_SCON_LIST_SHORT_MAX];
        scon_item_t* longItems;
    };
} _scon_list_t;

#define _SCON_LIST_ITEMS(_list) ((_list)->capacity <= _SCON_LIST_SHORT_MAX ? (_list)->shortItems : (_list)->longItems)

static inline void _scon_list_init(scon_t* scon, _scon_list_t* list);

static inline void _scon_list_deinit(scon_t* scon, _scon_list_t* list);

static inline _scon_list_t* _scon_list_new(scon_t* scon, scon_size_t capacity);

static inline void _scon_list_push_back(scon_t* scon, _scon_list_t* list, scon_item_t item);

static inline void _scon_list_remove_unstable(scon_t* scon, _scon_list_t* list, scon_item_t item);

#endif
