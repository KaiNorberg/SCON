#ifndef SCON_H
#define SCON_H 1

#include "closure.h"
#include "compile.h"
#include "core.h"
#include "disasm.h"
#include "eval.h"
#include "gc.h"
#include "handle.h"
#include "intrinsic.h"
#include "item.h"
#include "list.h"
#include "native.h"
#include "parse.h"
#include "stdlib.h"
#include "stringify.h"
#include "error.h"

#if defined(SCON_INLINE) || defined(SCON_IMPL)
#include "atom_impl.h"
#include "char_impl.h"
#include "closure_impl.h"
#include "compile_impl.h"
#include "core_impl.h"
#include "disasm_impl.h"
#include "eval_impl.h"
#include "function_impl.h"
#include "gc_impl.h"
#include "handle_impl.h"
#include "intrinsic_impl.h"
#include "item_impl.h"
#include "list_impl.h"
#include "native_impl.h"
#include "parse_impl.h"
#include "stdlib_impl.h"
#include "stringify_impl.h"
#include "error_impl.h"
#endif

#endif
