#include "pg_array_utils.h"

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
    if (ARR_NDIM(arg_array) <= 1 || ArrayGetNItems(ARR_NDIM(arg_array), ARR_DIMS(arg_array)) == 0)
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

