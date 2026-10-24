#include "pg_array_utils.h"

PG_FUNCTION_INFO_V1(array_pad);
PG_FUNCTION_INFO_V1(array_pad_arr);

Datum
array_pad(PG_FUNCTION_ARGS)
{
    ArrayType *arg_array, *res_array;
    Datum *arg_elements, *res_elements, element;
    Oid eltp;
    int16 typlen;
    bool typbyval, *arg_nulls, in_the_end;
    char typalign;
    int arg_nelems;
    int64 res_nelems;

    arg_array = PG_GETARG_ARRAYTYPE_P(0);
    if (ARR_NDIM(arg_array) > 1)
    {
        ereport(ERROR,
                (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
                 errmsg("The argument array should be one-dimensioned")));
    }

    eltp = PG_GETARG_TYPE(2);
    if (eltp != ARR_ELEMTYPE(arg_array))
    {
        ereport(ERROR,
                (errcode(ERRCODE_DATATYPE_MISMATCH),
                 errmsg("The element to add should have the same type as array elements")));
    }
    element = PG_GETARG_DATUM(2);

    res_nelems = PG_GETARG_INT32(1);
    if (res_nelems == 0) 
    {
        if (PG_ARRAY_IS_EMPTY(arg_array))
        {
            res_array = construct_empty_array(eltp);
            PG_RETURN_ARRAYTYPE_P(res_array);
        }
        else
        {
            ereport(ERROR,
                    (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
                     errmsg("The new lenght of the array can't be less than current")));
        }
    }

    in_the_end = res_nelems > 0;
    res_nelems *= (in_the_end) ? 1 : -1;
    if (res_nelems > MaxAllocSize / sizeof(Datum))
    {
        ereport(ERROR,
                (errcode(ERRCODE_PROGRAM_LIMIT_EXCEEDED),
                 errmsg("The resulting array is too large")));
    }

    res_elements = palloc(sizeof(Datum) * res_nelems);
    get_typlenbyvalalign(eltp, &typlen, &typbyval, &typalign);
    if (PG_ARRAY_IS_EMPTY(arg_array))
    {
        for (int i = 0; i < res_nelems; ++i)
        {
            res_elements[i] = element;
        }
        res_array = construct_array(res_elements, res_nelems, eltp, typlen, typbyval, typalign);
    }
    else
    {
        int delta, fin_index, shift, dims[1], lbs[1];
        bool* res_nulls = NULL;

        deconstruct_array(arg_array, eltp, typlen, typbyval, typalign, &arg_elements, &arg_nulls, &arg_nelems);

        delta = res_nelems - arg_nelems;
        shift = (in_the_end) ? 0 : delta;
        fin_index = (in_the_end) ? res_nelems : delta;
        if (delta < 0)
        {
            ereport(ERROR,
                    (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
                     errmsg("The new lenght of the array can't be less than current")));
        }

        memcpy(res_elements + shift, arg_elements, arg_nelems * sizeof(Datum));
        if (arg_nulls)
        {
            res_nulls = palloc0(sizeof(bool) * res_nelems);
            memcpy(res_nulls + shift, arg_nulls, arg_nelems * sizeof(bool));
        }
        for (int i = ((in_the_end) ? arg_nelems : 0); i < fin_index; ++i)
        {
            res_elements[i] = element;
        }
        dims[0] = res_nelems;
        lbs[0]  = ARR_LBOUND(arg_array)[0];
        res_array = construct_md_array(res_elements, res_nulls, 1, dims, lbs, eltp, typlen, typbyval, typalign);
        
        pfree(arg_elements);
        if (arg_nulls) 
        {
            pfree(arg_nulls);
            pfree(res_nulls);
        }
    }
    pfree(res_elements);

    PG_RETURN_ARRAYTYPE_P(res_array);
}

Datum
array_pad_arr(PG_FUNCTION_ARGS)
{
    ArrayType *arg_array, *res_array, *elem_array;
    Datum *arg_elements, *res_elements, *elem_elements;
    Oid eltp;
    int16 typlen;
    bool typbyval, *arg_nulls, *res_nulls, *elem_nulls, in_the_end, dims_corr, lbs_corr, arg_arr_empty;
    char typalign;
    int arg_nelems, elem_nelems;
    int64 size, delta;
    int arg_ndim, elem_ndim, *arg_dims, *elem_dims, *arg_lbs, *elem_lbs, res_ndim, *res_dims, *res_lbs;
    Size szene, res_nelems, datumXelem_nelems, boolXelem_nelems;

    arg_array = PG_GETARG_ARRAYTYPE_P(0);
    size = PG_GETARG_INT32(1);
    in_the_end = size > 0;
    size *= (in_the_end) ? 1 : -1;
    elem_array = PG_GETARG_ARRAYTYPE_P(2);

    arg_ndim = ARR_NDIM(arg_array);
    arg_dims = ARR_DIMS(arg_array);
    arg_lbs = ARR_LBOUND(arg_array);
    elem_ndim = ARR_NDIM(elem_array);
    elem_dims = ARR_DIMS(elem_array);
    elem_lbs = ARR_LBOUND(elem_array);

    arg_nelems = 0; delta = size;

    if (pgau_array_is_empty(elem_ndim, elem_dims))
    {
        ereport(ERROR,
                (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
                 errmsg("The array being added to the original array shouldn't be empty")));
    }
    eltp = ARR_ELEMTYPE(elem_array);
    if (eltp != ARR_ELEMTYPE(arg_array))
    {
        ereport(ERROR,
                (errcode(ERRCODE_DATATYPE_MISMATCH),
                 errmsg("The array being added to the original array should have the same type as the original")));
    }
    get_typlenbyvalalign(eltp, &typlen, &typbyval, &typalign);

    arg_arr_empty = pgau_array_is_empty(arg_ndim, arg_dims);
    if (!arg_arr_empty)
    {
        int ind;

        if (arg_ndim - 1 != elem_ndim)
        {
            ereport(ERROR,
                    (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
                     errmsg("The array being added to the original array should have one dimension fewer than the original")));
        }
        dims_corr = true; ind = 0;
        while (ind < elem_ndim && dims_corr) { dims_corr &= arg_dims[ind + 1] == elem_dims[ind]; ++ind; }
        if (!dims_corr) 
        {
            ereport(ERROR,
                    (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
                     errmsg("The array being added to the original array should have a numbers of elements equal to the numbers of elements in the original array in all dimensions except the first")));
        }
        lbs_corr = true; ind = 0;
        while (ind < elem_ndim && lbs_corr) { lbs_corr &= arg_lbs[ind + 1] == elem_lbs[ind]; ++ind; }
        if (!lbs_corr) 
        {
            ereport(ERROR,
                    (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
                     errmsg("The array being added to the original array should have a lower bounds equal to the lower bounds in the original array in all dimensions except the first")));
        }
        deconstruct_array(arg_array, eltp, typlen, typbyval, typalign, &arg_elements, &arg_nulls, &arg_nelems);
        delta = size - arg_dims[0];
    }
    deconstruct_array(elem_array, eltp, typlen, typbyval, typalign, &elem_elements, &elem_nulls, &elem_nelems);
    
    if (delta < 0)
    {
        ereport(ERROR,
                (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
                 errmsg("The new lenght of the array can't be less than current")));
    }
    if (size > (int)(MaxAllocSize / sizeof(Datum) / elem_nelems))
    {
        ereport(ERROR,
                (errcode(ERRCODE_PROGRAM_LIMIT_EXCEEDED),
                 errmsg("Resulting array is too large"))); 
    }
    if (size == 0) 
    {
        res_array = construct_empty_array(eltp);
        pfree(elem_elements);
        pfree(elem_nulls);
        PG_RETURN_ARRAYTYPE_P(res_array);
    }

    szene = (Size)elem_nelems;
    res_nelems = (Size)size * szene;
    datumXelem_nelems = sizeof(Datum) * szene;
    boolXelem_nelems = sizeof(bool) * szene;
    
    res_elements = palloc(sizeof(Datum) * res_nelems);
    res_nulls = (elem_nulls || (!arg_arr_empty && arg_nulls)) ? palloc0(sizeof(bool) * res_nelems) : NULL;

    res_ndim = elem_ndim + 1;

    res_dims = palloc(sizeof(int) * res_ndim);
    memcpy(res_dims + 1, elem_dims, sizeof(int) * elem_ndim);
    res_dims[0] = size;

    res_lbs = palloc(sizeof(int) * res_ndim);
    memcpy(res_lbs + 1, elem_lbs, sizeof(int) * elem_ndim);
    res_lbs[0] = (arg_arr_empty) ? 1 : arg_lbs[0];

    if (arg_arr_empty)
    {
        for (Size i = 0; i < res_nelems; i += elem_nelems)
            memcpy(res_elements + i, elem_elements, datumXelem_nelems);
        if (elem_nulls)
            for (Size i = 0; i < res_nelems; i += elem_nelems)
                memcpy(res_nulls + i, elem_nulls, boolXelem_nelems);
    }
    else
    {
        Size shift, fin_index;

        shift = ((in_the_end) ? 0 : (Size)delta) * szene;
        fin_index = (in_the_end) ? res_nelems : (Size)delta * szene;
    
        memcpy(res_elements + shift, arg_elements, arg_nelems * sizeof(Datum));
        if (arg_nulls) memcpy(res_nulls + shift, arg_nulls, arg_nelems * sizeof(bool));

        for (Size i = ((in_the_end) ? (Size)arg_nelems : 0); i < fin_index; i += szene)
            memcpy(res_elements + i, elem_elements, datumXelem_nelems);
        if (elem_nulls)
            for (Size i = ((in_the_end) ? (Size)arg_nelems : 0); i < fin_index; i += szene)
                memcpy(res_nulls + i, elem_nulls, boolXelem_nelems);
    }
    res_array = construct_md_array(res_elements, res_nulls, res_ndim, res_dims, res_lbs, eltp, typlen, typbyval, typalign);

    pfree(elem_elements);
    if (elem_nulls) pfree(elem_nulls);
    if (!arg_arr_empty)
    {
        pfree(arg_elements);
        if (arg_nulls) pfree(arg_nulls);
    }
    pfree(res_elements);
    if (res_nulls) pfree(res_nulls);
    pfree(res_dims);
    pfree(res_lbs);

    PG_RETURN_ARRAYTYPE_P(res_array);
}

