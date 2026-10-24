#pragma once

#include "postgres.h"
#include "fmgr.h"
#include "utils/array.h"
#include <string.h>
#include "catalog/pg_type.h"
#include "utils/builtins.h"
#include "utils/numeric.h"
#include "utils/datum.h"
#include "utils/float.h"
#include "utils/memutils.h"
#include "utils/errcodes.h"
#include "utils/lsyscache.h"
#include "utils/json.h"

#define PG_GETARG_TYPE(n) get_fn_expr_argtype(fcinfo->flinfo, (n))
#define PG_ARRAY_IS_EMPTY(arr) ARR_NDIM(arr) == 0 || ArrayGetNItems(ARR_NDIM(arr), ARR_DIMS(arr)) == 0

/*
Checks does the type represent a number from a human perspective (int, float, etc.).
*/
static inline bool 
pgau_is_numeric_type(Oid eltp)
{
    switch(eltp)
    {
        case INT2OID:
        case INT4OID:
        case INT8OID:
        case FLOAT4OID:
        case FLOAT8OID:
        case NUMERICOID:
        case OIDOID:
        case MONEYOID: return true;
        default: return false;
    }
}

/*
Checks does the type represent a text from a human perspective (text, varchar, bytea, etc.).
*/
static inline bool 
pgau_is_text_type(Oid eltp)
{
    switch(eltp)
    {
        case TEXTOID:
        case VARCHAROID:
        case BPCHAROID:
        case CHAROID:
        case NAMEOID:
        case BYTEAOID: return true;
        default: return false;
    }
}

/*
Requires number of dimensions (NDIM) and pointer to the array of dimensions sizes (DIMS) of the array and returns TRUE if array is empty.
*/
static inline bool 
pgau_array_is_empty(int ndim, int* dims) { return ndim == 0 || ArrayGetNItems(ndim, dims) == 0; }

/*
Appends N chars C into StringInfo STR.
*/
static inline void
appendStringInfoNChars(StringInfo str, char c, int n)
{
    if (n <= 0) return;
    enlargeStringInfo(str, n);
    memset(str->data + str->len, c, n);
    str->len += n;
    str->data[str->len] = '\0';
}


bool
pgau_numeric_is_zero(Oid eltp, Datum a);
bool
pgau_text_is_empty(Oid eltp, Datum a);

char*
pgau_datum_to_json_cstring(Datum value, Oid type);
void
pgau_array_to_StringInfo(StringInfo dest, Datum* elems, bool* nulls, Oid eltp, int nelems, int ndim, int* dims);

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

