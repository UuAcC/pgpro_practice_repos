#include "pg_array_utils.h"

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
        ereport(ERROR,
                (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
                 errmsg("The argument array should be one-dimensional")));
    }
    if (PG_ARRAY_IS_EMPTY(arg_array))
    {
        ereport(ERROR,
                (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
                 errmsg("The argument array can't be empty")));
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

