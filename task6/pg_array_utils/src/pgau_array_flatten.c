#include "pg_array_utils.h"

PG_FUNCTION_INFO_V1(array_flatten);

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
    int arg_nelems, dims[1], lbs[1];

    arg_array = PG_GETARG_ARRAYTYPE_P(0);
    if (PG_ARRAY_IS_EMPTY(arg_array))
    {
        PG_RETURN_ARRAYTYPE_P(arg_array);
    }
    element_type = ARR_ELEMTYPE(arg_array);
    get_typlenbyvalalign(element_type, &typlen, &typbyval, &typalign);
    deconstruct_array(arg_array, element_type, typlen, typbyval, typalign, &arg_elements, &arg_nulls, &arg_nelems);

    dims[0] = arg_nelems;
    lbs[0]  = 1;

    res_array = construct_md_array(arg_elements, arg_nulls,
                                1, dims, lbs,
                                element_type, typlen, typbyval, typalign);
    pfree(arg_elements);
    pfree(arg_nulls);
    
    PG_RETURN_ARRAYTYPE_P(res_array);
}

