#include "pg_array_utils.h"

PG_FUNCTION_INFO_V1(array_slice);

/*
 * anyarray array_slice ( anyarray, int start, int end, int step = 1 )
 *
 * start/end are 1-based positions within the array.
 *      start = 1  -> first element, start = -1 -> last element
 * lbs[0] of the input array is ignored; the result always has lbs[0] = 1.
 */
Datum
array_slice(PG_FUNCTION_ARGS)
{
    ArrayType *arg_array, *res_array;
    Oid eltp;
    int16 typlen;
    bool typbyval, *arg_nulls, *res_nulls;
    char typalign;
    Datum *arg_elements, *res_elements;
    int arg_nelems, step, ndim, *res_dims, *res_lbs;
    int64 start, end;
    Size element_size, res_index, res_nelems;
    Size _start, _end, _step, datumXelement_size, boolXelement_size;

    arg_array = PG_GETARG_ARRAYTYPE_P(0);
    start = PG_GETARG_INT32(1);
    end = PG_GETARG_INT32(2);
    step = PG_GETARG_INT32(3);

    ndim = ARR_NDIM(arg_array);
    if (PG_ARRAY_IS_EMPTY(arg_array))
    {
        ereport(ERROR,
                (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
                 errmsg("The argument array can't be empty")));
    }
    res_dims = palloc(sizeof(int) * ndim);
    res_lbs  = palloc(sizeof(int) * ndim);
    memcpy(res_dims, ARR_DIMS(arg_array), sizeof(int) * ndim);
    memcpy(res_lbs,  ARR_LBOUND(arg_array), sizeof(int) * ndim);

    if (step <= 0) 
    {
        ereport(ERROR,
                (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
                 errmsg("The step should be greater than zero")));
    }

    start = (start > 0) ? start - 1 : start + res_dims[0];
    end = (end > 0) ? end - 1 : end + res_dims[0];
    if (start < 0 || start > end) 
    {
        ereport(ERROR,
                (errcode(ERRCODE_ARRAY_SUBSCRIPT_ERROR),
                 errmsg("The start index should be between 1 and the end index")));
    }
    if (end >= res_dims[0]) 
    {
        ereport(ERROR,
                (errcode(ERRCODE_ARRAY_SUBSCRIPT_ERROR),
                 errmsg("The end index should be between the start index and the lenght of the first dimension of the array")));
    }

    eltp = ARR_ELEMTYPE(arg_array);
    get_typlenbyvalalign(eltp, &typlen, &typbyval, &typalign);
    deconstruct_array(arg_array, eltp, typlen, typbyval, typalign, &arg_elements, &arg_nulls, &arg_nelems);
    (void)arg_nelems;

    element_size = 1; res_index = 0;
    for (int i = 1; i < ndim; ++i) element_size *= (Size)res_dims[i];
    res_dims[0] = (end - start) / step + 1;
    res_lbs[0] = 1;
    res_nelems = element_size * (Size)res_dims[0];
    res_elements = palloc(sizeof(Datum) * res_nelems);
    res_nulls = palloc(sizeof(bool) * res_nelems);
    
    _start = start * element_size;
    _end = end * element_size;
    _step = step * element_size;
    datumXelement_size = sizeof(Datum) * element_size;
    boolXelement_size = sizeof(bool) * element_size;
    for (Size i = _start; i <= _end; i = i + _step)
    {
        memcpy(res_elements + res_index, arg_elements + i, datumXelement_size);
        if (arg_nulls) memcpy(res_nulls + res_index, arg_nulls + i, boolXelement_size);
        else memset(res_nulls + res_index, false, boolXelement_size);
        res_index += element_size;
    }
    
    res_array = construct_md_array(res_elements, res_nulls, ndim, res_dims, res_lbs, eltp, typlen, typbyval, typalign);

    pfree(res_dims);
    pfree(res_lbs);
    pfree(res_elements);
    pfree(res_nulls);
    pfree(arg_elements);
    if (arg_nulls) pfree(arg_nulls);

    PG_RETURN_ARRAYTYPE_P(res_array);
}

