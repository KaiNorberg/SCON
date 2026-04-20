#ifndef SCON_H
#define SCON_H 1

#include "builtin.h"
#include "compile.h"
#include "core.h"
#include "disasm.h"
#include "gc.h"
#include "handle.h"
#include "item.h"
#include "keyword.h"
#include "list.h"
#include "native.h"
#include "parse.h"

#if defined(SCON_INLINE) || defined(SCON_IMPL)
#include "atom_impl.h"
#include "builtin_impl.h"
#include "char_impl.h"
#include "compile_impl.h"
#include "core_impl.h"
#include "disasm_impl.h"
#include "function_impl.h"
#include "gc_impl.h"
#include "handle_impl.h"
#include "item_impl.h"
#include "keyword_impl.h"
#include "list_impl.h"
#include "native_impl.h"
#include "parse_impl.h"
#endif

#endif
