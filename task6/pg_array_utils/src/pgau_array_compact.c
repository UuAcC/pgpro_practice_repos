#include "pg_array_utils.h"

PG_FUNCTION_INFO_V1(array_compact);

Datum
array_compact(PG_FUNCTION_ARGS)
{
    text* arg_text = PG_GETARG_TEXT_PP(1);
    char* working_mode = text_to_cstring(arg_text);

    if (strcmp(working_mode, "null") == 0 || // does the entered mode exist
        strcmp(working_mode, "falsy") == 0 || 
        strcmp(working_mode, "zero") == 0 || 
        strcmp(working_mode, "empty") == 0)
    {
        ArrayType *arg_array, *res_array;
        Oid element_type;
        int16 typlen;
        bool typbyval, *arg_nulls;
        char typalign;
        Datum *arg_elements, *res_elems;
        int arg_nelems, new_nelems;
        bool wrong_mode = false;
        
        arg_array = PG_GETARG_ARRAYTYPE_P(0);
        element_type = ARR_ELEMTYPE(arg_array);
        if (strcmp(working_mode, "zero") == 0) // does the mode match the element_type
        {
            wrong_mode = !pgau_is_numeric_type(element_type);
        }
        else if (strcmp(working_mode, "empty") == 0)
        {
            wrong_mode = !pgau_is_text_type(element_type);
        }
        if (wrong_mode) 
        {
            ereport(ERROR,
                    (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
                     errmsg("Impossible to use %s mode with %s array", working_mode, format_type_be(element_type))));
        }
        if (PG_ARRAY_IS_EMPTY(arg_array)) // empty arg_array
        {
            pfree(working_mode);
            PG_RETURN_ARRAYTYPE_P(arg_array);
        }
        else if (ARR_NDIM(arg_array) > 1) // not a one-dimensional arg_array
        {
            ereport(ERROR,
                    (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
                     errmsg("The argument array must be one-dimensional")));
        }

        get_typlenbyvalalign(element_type, &typlen, &typbyval, &typalign);
        deconstruct_array(arg_array, element_type, typlen, typbyval, typalign, &arg_elements, &arg_nulls, &arg_nelems);

        new_nelems = 0;
        res_elems = palloc(sizeof(Datum) * arg_nelems);
        if (strcmp(working_mode, "null") == 0)
        {
            for (int i = 0; i < arg_nelems; ++i)
            {
                if (arg_nulls[i]) continue;
                res_elems[new_nelems] = arg_elements[i];
                ++new_nelems;
            }
            if (new_nelems == 0)
            {
                pfree(arg_elements);
                pfree(arg_nulls);
                pfree(res_elems);
                pfree(working_mode);
                PG_RETURN_ARRAYTYPE_P(construct_empty_array(element_type));
            } 
            // res_elems = repalloc(res_elems, sizeof(Datum) * new_nelems);
            res_array = construct_array(res_elems, new_nelems, element_type, typlen, typbyval, typalign);
        }
        else 
        {
            bool (*cmp_function)(Oid eltp, Datum a) = 
                (pgau_is_numeric_type(element_type)) ? pgau_numeric_is_zero : 
                ((pgau_is_text_type(element_type)) ? pgau_text_is_empty : NULL);
            if (strcmp(working_mode, "falsy") == 0)
            {
                if (!cmp_function) // element_type == BOOLOID || element_type == (neither text nor numeric type)
                {
                    for (int i = 0; i < arg_nelems; ++i)
                    {
                        if (arg_nulls[i] || (element_type == BOOLOID && !DatumGetBool(arg_elements[i]))) continue;
                        res_elems[new_nelems] = arg_elements[i];
                        ++new_nelems;
                    }
                }
                else // element_type == text or numeric type
                {
                    for (int i = 0; i < arg_nelems; ++i)
                    {
                        if (arg_nulls[i]) continue;
                        if (cmp_function(element_type, arg_elements[i])) continue; 
                        res_elems[new_nelems] = arg_elements[i];
                        ++new_nelems;
                    }
                }
                if (new_nelems == 0)
                {
                    pfree(arg_elements);
                    pfree(arg_nulls);
                    pfree(res_elems);
                    pfree(working_mode);
                    PG_RETURN_ARRAYTYPE_P(construct_empty_array(element_type));
                } 
                // res_elems = repalloc(res_elems, sizeof(Datum) * new_nelems);
                res_array = construct_array(res_elems, new_nelems, element_type, typlen, typbyval, typalign);
            }
            else // element_type == text or numeric type, but res_array can have NULLs
            {
                int dims[1], lbs[1];
                bool *res_nulls = palloc0(sizeof(bool) * arg_nelems);

                for (int i = 0; i < arg_nelems; ++i)
                {
                    if (!arg_nulls[i])
                    {
                        if (cmp_function(element_type, arg_elements[i])) continue;
                        res_elems[new_nelems] = arg_elements[i];
                        ++new_nelems;
                    }
                    else
                    {
                        res_nulls[new_nelems] = true;
                        ++new_nelems;
                    }
                }
                if (new_nelems == 0)
                {
                    pfree(arg_elements);
                    pfree(arg_nulls);
                    pfree(res_elems);
                    pfree(res_nulls);
                    pfree(working_mode);
                    PG_RETURN_ARRAYTYPE_P(construct_empty_array(element_type));
                } 
                // res_elems = repalloc(res_elems, sizeof(Datum) * new_nelems);
                // res_nulls = repalloc(res_nulls, sizeof(bool) * new_nelems);
                dims[0] = new_nelems; lbs[0] = 1;
                res_array = construct_md_array(res_elems, res_nulls, 1, dims, lbs, element_type, typlen, typbyval, typalign);
                pfree(res_nulls);
            }
        }
        pfree(arg_elements);
        pfree(arg_nulls);
        pfree(res_elems);
        pfree(working_mode);
        PG_RETURN_ARRAYTYPE_P(res_array);
    }
    else 
    {
        ereport(ERROR,
                (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
                 errmsg("%s is not an existing mode", working_mode)));
    }
}

