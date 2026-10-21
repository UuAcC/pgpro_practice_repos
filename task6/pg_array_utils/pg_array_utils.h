#pragma once

#include "postgres.h"
#include "fmgr.h"
#include "utils/array.h"
#include <string.h>
#include "pg_type_d.h"
#include "utils/builtins.h"
#include "utils/numeric.h"
#include "utils/datum.h"
#include "utils/float.h"
#include "utils/memutils.h"

#ifdef PG_MODULE_MAGIC
PG_MODULE_MAGIC;
#endif

PG_FUNCTION_INFO_V1(array_flatten); // ++
PG_FUNCTION_INFO_V1(array_chunk); // ++
PG_FUNCTION_INFO_V1(array_zip);
PG_FUNCTION_INFO_V1(array_compact); // ++
PG_FUNCTION_INFO_V1(array_pad);
PG_FUNCTION_INFO_V1(array_pad_arr);
PG_FUNCTION_INFO_V1(array_slice); // ++
PG_FUNCTION_INFO_V1(array_repeat); // ++
PG_FUNCTION_INFO_V1(array_repeat_arr); // ++

#define PG_GETARG_TYPE(n) get_fn_expr_argtype(fcinfo->flinfo, (n))

static inline bool 
pgau_is_numeric_type(Oid eltp);
static inline bool 
pgau_is_text_type(Oid eltp);
static inline bool
pgau_numeric_is_zero(Oid eltp, Datum a);
static inline bool
pgau_text_is_empty(Oid eltp, Datum a);

Datum
array_flatten(PG_FUNCTION_ARGS);

Datum
array_chunk(PG_FUNCTION_ARGS);

Datum
array_zip(PG_FUNCTION_ARGS);

Datum
array_compact(PG_FUNCTION_ARGS);

Datum
array_pad(PG_FUNCTION_ARGS);
Datum
array_pad_arr(PG_FUNCTION_ARGS);

Datum
array_slice(PG_FUNCTION_ARGS);

Datum
array_repeat(PG_FUNCTION_ARGS);
Datum
array_repeat_arr(PG_FUNCTION_ARGS);
