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

        ArrayIterator arg_array_iter;
        bool iargelm_isnull;
        Datum iargelm_datum;
        
        Oid element_type;
        int16 typlen;
        bool typbyval;
        char typalign;

        Datum *res_elems;
        int new_nelems;

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
        get_typlenbyvalalign(element_type, &typlen, &typbyval, &typalign);

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

        arg_array_iter = array_create_iterator(arg_array, 0, NULL);

        new_nelems = 0;
        res_elems = palloc(sizeof(Datum) * ARR_DIMS(arg_array)[0]);
        if (strcmp(working_mode, "null") == 0)
        {
            while (array_iterate(arg_array_iter, &iargelm_datum, &iargelm_isnull))
            {
                if (iargelm_isnull) continue;
                res_elems[new_nelems] = iargelm_datum;
                ++new_nelems;
            }
            if (new_nelems == 0)
            {
                array_free_iterator(arg_array_iter);
                pfree(res_elems);
                pfree(working_mode);
                PG_RETURN_ARRAYTYPE_P(construct_empty_array(element_type));
            } 
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
                    while (array_iterate(arg_array_iter, &iargelm_datum, &iargelm_isnull))
                    {
                        if (iargelm_isnull || (element_type == BOOLOID && !DatumGetBool(iargelm_datum))) continue;
                        res_elems[new_nelems] = iargelm_datum;
                        ++new_nelems;
                    }
                }
                else // element_type == text or numeric type
                {
                    while (array_iterate(arg_array_iter, &iargelm_datum, &iargelm_isnull))
                    {
                        if (iargelm_isnull) continue;
                        if (cmp_function(element_type, iargelm_datum)) continue; 
                        res_elems[new_nelems] = iargelm_datum;
                        ++new_nelems;
                    }
                }
                if (new_nelems == 0)
                {
                    array_free_iterator(arg_array_iter);
                    pfree(res_elems);
                    pfree(working_mode);
                    PG_RETURN_ARRAYTYPE_P(construct_empty_array(element_type));
                } 
                res_array = construct_array(res_elems, new_nelems, element_type, typlen, typbyval, typalign);
            }
            else // element_type == text or numeric type, but res_array can have NULLs
            {
                int dims[1], lbs[1];
                bool *res_nulls = palloc0(sizeof(bool) * ARR_DIMS(arg_array)[0]);

                while (array_iterate(arg_array_iter, &iargelm_datum, &iargelm_isnull))
                {
                    if (!iargelm_isnull)
                    {
                        if (cmp_function(element_type, iargelm_datum)) continue;
                        res_elems[new_nelems] = iargelm_datum;
                    }
                    else 
                    {
                        res_nulls[new_nelems] = true;
                    }
                    ++new_nelems;
                }
                if (new_nelems == 0)
                {
                    array_free_iterator(arg_array_iter);
                    pfree(res_elems);
                    pfree(res_nulls);
                    pfree(working_mode);
                    PG_RETURN_ARRAYTYPE_P(construct_empty_array(element_type));
                } 
                dims[0] = new_nelems; lbs[0] = 1;
                res_array = construct_md_array(res_elems, res_nulls, 1, dims, lbs, element_type, typlen, typbyval, typalign);
                pfree(res_nulls);
            }
        }
        array_free_iterator(arg_array_iter);
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

