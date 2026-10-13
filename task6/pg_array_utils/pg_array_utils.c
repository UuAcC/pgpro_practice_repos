#include "postgres.h"
#include "fmgr.h"
#include "utils/array.h"
#include <string.h>
#include "pg_type_d.h"
#include "utils/builtins.h"

#ifdef PG_MODULE_MAGIC
PG_MODULE_MAGIC;
#endif

PG_FUNCTION_INFO_V1(array_flatten);
PG_FUNCTION_INFO_V1(array_chunk);

Datum
array_flatten(PG_FUNCTION_ARGS)
{
    ArrayType *arg_array, *res_array;

    Oid element_type;
    int16 typlen;
    bool typbyval;
    char typalign;

    Datum *arg_elements;
    bool *arg_nulls;
    int arg_nelems;

    arg_array = PG_GETARG_ARRAYTYPE_P(0);
    if (ARR_NDIM(arg_array) == 1)
    {
        PG_RETURN_ARRAYTYPE_P(arg_array);
    }
    element_type = ARR_ELEMTYPE(arg_array);
    get_typlenbyvalalign(element_type, &typlen, &typbyval, &typalign);
    deconstruct_array(arg_array, element_type, typlen, typbyval, typalign, &arg_elements, &arg_nulls, &arg_nelems);

    res_array = construct_array(arg_elements, arg_nelems, element_type, typlen, typbyval, typalign);

    pfree(arg_elements);
    pfree(arg_nulls);
    PG_RETURN_ARRAYTYPE_P(res_array);
}

Datum
array_chunk(PG_FUNCTION_ARGS) 
{
    ArrayType *arg_array, *res_array;

    Oid element_type;
    int16 typlen;
    bool typbyval;
    char typalign;

    Datum *elements;
    bool *nulls;
    int arg_nelems, chunk_size, chunks_cnt, new_size;

    arg_array = PG_GETARG_ARRAYTYPE_P(0);
    if (ARR_NDIM(arg_array) > 1)
    {
        elog(ERROR, "The argument array must be one-dimensional");
    }
    element_type = ARR_ELEMTYPE(arg_array);
    get_typlenbyvalalign(element_type, &typlen, &typbyval, &typalign);
    deconstruct_array(arg_array, element_type, typlen, typbyval, typalign, &elements, &nulls, &arg_nelems);

    chunk_size = PG_GETARG_INT32(1);
    chunks_cnt = (arg_nelems + chunk_size - 1) / chunk_size;
    new_size = chunk_size * chunks_cnt;

    int dims[2] = {chunks_cnt, chunk_size};
    int lbs[2] = {1, 1};

    elements = repalloc(elements, sizeof(Datum) * new_size);
    nulls = repalloc(nulls, sizeof(bool) * new_size);
    memset(nulls + arg_nelems, true, (new_size - arg_nelems) * sizeof(bool));

    res_array = construct_md_array(elements, nulls, 2, dims, lbs, element_type, typlen, typbyval, typalign);

    pfree(elements);
    pfree(nulls);
    PG_RETURN_ARRAYTYPE_P(res_array);
}

static bool is_number(Oid eltp)
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
        case CASHOID: return true;
        default: return false;
    }
}

static bool is_text(Oid eltp)
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

Datum
array_compact(PG_FUNCTION_ARGS)
{
    text* arg_text = PG_GETARG_TEXT_PP(1);
    char* working_mode = text_to_cstring(arg_text);

    if (working_mode == "null" || 
        working_mode == "falsy" || 
        working_mode == "zero" || 
        working_mode == "empty")
    {
        ArrayType *arg_array;
        Oid element_type;
        char* element_type_name;
        bool wrong_mode = false;
        
        arg_array = PG_GETARG_ARRAYTYPE_P(0);
        element_type = ARR_ELEMTYPE(arg_array);
        element_type_name = format_type_be(element_type);
        if (working_mode == "zero")
        {
            wrong_mode = !is_number(element_type);
        }
        else if (working_mode == "empty")
        {
            wrong_mode = !is_text(element_type);
        }
        else if (working_mode == "falsy")
        {
            wrong_mode = !(element_type == BOOLOID || is_number(element_type) || is_text(element_type));
        }
        if (wrong_mode) elog(ERROR, "Impossible to use %s mode with %s array", working_mode, element_type_name);
        pfree(element_type_name);

        ArrayType *res_array;
        
        int16 typlen;
        bool typbyval;
        char typalign;

        Datum *arg_elements;
        bool *arg_nulls;
        int arg_nelems, ndims, *dims, *lbs;

        ndims = ARR_NDIM(arg_array);
        dims = ARR_DIMS(arg_array);
        lbs = ARR_LBOUND(arg_array);
        get_typlenbyvalalign(element_type, &typlen, &typbyval, &typalign);
        deconstruct_array(arg_array, element_type, typlen, typbyval, typalign, &arg_elements, &arg_nulls, &arg_nelems);
        if (working_mode == "null")
        {
            int new_nelems = 0, i;
            Datum* res_elems = palloc(sizeof(Datum) * arg_nelems);
            for (i = 0; i < arg_nelems; ++i)
            {
                if (!arg_nulls[i])
                {
                    res_elems[new_nelems] = arg_elements[i];
                    ++new_nelems;
                }
            }
            ++new_nelems;
            res_elems = repalloc(res_elems, sizeof(Datum) * new_nelems);
        }
        else if (working_mode == "zero")
        {

        }
        else if (working_mode == "empty")
        {

        }
        else 
        {

        }
    }
    else elog(ERROR, "%s is not an existing mode", working_mode);
    pfree(working_mode);
}

