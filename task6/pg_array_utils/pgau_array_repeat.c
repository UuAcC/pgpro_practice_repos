#include "pg_array_utils.h"

PG_FUNCTION_INFO_V1(array_repeat);
PG_FUNCTION_INFO_V1(array_repeat_arr);

Datum
array_repeat(PG_FUNCTION_ARGS)
{
    ArrayType* res_array;
    Datum arg_element, *res_elems;
    Oid eltp;
    int16 typlen;
    bool typbyval;
    char typalign;
    int count;
    arg_element = PG_GETARG_DATUM(0);
    eltp = PG_GETARG_TYPE(0);
    count = PG_GETARG_INT32(1);
    if (count < 0)
    {
        ereport(ERROR,
                (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
                 errmsg("Count can't be negative")));
    }
    if (count == 0)
        PG_RETURN_ARRAYTYPE_P(construct_empty_array(eltp));
    if (count > MaxAllocSize / sizeof(Datum))
    {
        ereport(ERROR,
                (errcode(ERRCODE_PROGRAM_LIMIT_EXCEEDED),
                 errmsg("Resulting array is too large")));
    }
    res_elems = palloc(sizeof(Datum) * count);
    for (int i = 0; i < count; ++i)
        res_elems[i] = arg_element;
    get_typlenbyvalalign(eltp, &typlen, &typbyval, &typalign);
    res_array = construct_array(res_elems, count, eltp, typlen, typbyval, typalign);
    pfree(res_elems);
    PG_RETURN_ARRAYTYPE_P(res_array);
}

Datum
array_repeat_arr(PG_FUNCTION_ARGS)
{
    ArrayType *arg_array, *res_array;
    Datum *arg_elements, *res_elements, *res_elems_moving_pointer;
    Oid eltp;
    int16 typlen;
    bool typbyval, *arg_nulls, *res_nulls, *res_nulls_moving_pointer;
    char typalign;
    int count, arg_nelems, arg_ndim, *arg_dims, *arg_lbs, res_ndim, *res_dims, *res_lbs;
    Size datumXarg_nelems, boolXarg_nelems, res_nelems;

    arg_array = PG_GETARG_ARRAYTYPE_P(0);
    eltp = ARR_ELEMTYPE(arg_array);
    count = PG_GETARG_INT32(1);
    if (count < 0)
    {
        ereport(ERROR,
                (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
                 errmsg("Count can't be negative")));
    }
    if (count == 0 || PG_ARRAY_IS_EMPTY(arg_array))
        PG_RETURN_ARRAYTYPE_P(construct_empty_array(eltp));

    arg_ndim = ARR_NDIM(arg_array);
    arg_dims = ARR_DIMS(arg_array);
    arg_lbs = ARR_LBOUND(arg_array);
    
    get_typlenbyvalalign(eltp, &typlen, &typbyval, &typalign);
    deconstruct_array(arg_array, eltp, typlen, typbyval, typalign, &arg_elements, &arg_nulls, &arg_nelems);

    if (arg_nelems > (int) (MaxAllocSize / sizeof(Datum) / count))
    {
        ereport(ERROR,
                (errcode(ERRCODE_PROGRAM_LIMIT_EXCEEDED),
                 errmsg("Resulting array is too large")));
    }
    
    res_nelems = (Size)arg_nelems * (Size)count;
    res_elements = palloc(sizeof(Datum) * res_nelems);
    res_nulls = (arg_nulls) ? palloc(sizeof(bool) * res_nelems) : NULL;

    datumXarg_nelems = sizeof(Datum) * (Size)arg_nelems;
    boolXarg_nelems = sizeof(bool) * (Size)arg_nelems;
    res_elems_moving_pointer = res_elements;
    res_nulls_moving_pointer = res_nulls;
    for (int i = 0; i < count; ++i)
    {
        memcpy(res_elems_moving_pointer, arg_elements, datumXarg_nelems);
        res_elems_moving_pointer += arg_nelems;
        if (arg_nulls)
        {
            memcpy(res_nulls_moving_pointer, arg_nulls, boolXarg_nelems);
            res_nulls_moving_pointer += arg_nelems;
        }
    }

    res_ndim = arg_ndim + 1;
    res_dims = palloc(sizeof(int) * res_ndim);
    res_lbs = palloc(sizeof(int) * res_ndim);
    res_dims[0] = count;
    res_lbs[0] = 1;
    memcpy(res_dims + 1, arg_dims, sizeof(int) * arg_ndim);
    memcpy(res_lbs + 1, arg_lbs, sizeof(int) * arg_ndim);

    res_array = construct_md_array(res_elements, res_nulls, res_ndim, res_dims, res_lbs, eltp, typlen, typbyval, typalign);

    pfree(arg_elements);
    pfree(res_elements);
    if (arg_nulls) 
    {
        pfree(arg_nulls);
        pfree(res_nulls);
    }
    pfree(res_dims);
    pfree(res_lbs);

    PG_RETURN_ARRAYTYPE_P(res_array);
}

