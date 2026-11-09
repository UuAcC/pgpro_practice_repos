#include "pg_array_utils.h"

PG_FUNCTION_INFO_V1(array_zip);

Datum
array_zip(PG_FUNCTION_ARGS)
{
    ArrayType** arg_arrays, *res_array;
    ArrayIterator* array_iters;
    Datum *jsonb_datums;
    Oid* arg_arrays_eltps;
    int* arg_arrays_ndims;
    int max_array_len, arg_arrays_count;
    int16 res_typlen;
    bool  res_typbyval;
    char  res_typalign;
    int res_dims[1], res_lbs[1];

    arg_arrays_count = PG_NARGS();

    if (arg_arrays_count == 0)
    {
        ereport(ERROR,
                (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
                 errmsg("array_zip requires at least one array")));
    }
    arg_arrays = palloc(sizeof(ArrayType*) * arg_arrays_count);
    arg_arrays_eltps = palloc(sizeof(Oid) * arg_arrays_count);
    arg_arrays_ndims = palloc(sizeof(int) * arg_arrays_count);
    array_iters = palloc(sizeof(ArrayIterator) * arg_arrays_count);

    max_array_len = 0;

    for (int i = 0; i < arg_arrays_count; ++i)
    {
        int* idims;
        bool arr_is_emp;

        arg_arrays[i] = PG_GETARG_ARRAYTYPE_P(i);
        arg_arrays_eltps[i] = ARR_ELEMTYPE(arg_arrays[i]);
        arg_arrays_ndims[i] = ARR_NDIM(arg_arrays[i]);
        idims = ARR_DIMS(arg_arrays[i]);
        
        arr_is_emp = pgau_array_is_empty(arg_arrays_ndims[i], idims);
        array_iters[i] = array_create_iterator(arg_arrays[i], (arr_is_emp ? 0 : arg_arrays_ndims[i] - 1), NULL);
        max_array_len = (!arr_is_emp && (idims[0] > max_array_len)) ? idims[0] : max_array_len;
    }

    if (max_array_len == 0)
    {
        pfree(arg_arrays);
        pfree(arg_arrays_eltps);
        pfree(arg_arrays_ndims);
        for (int i = 0; i < arg_arrays_count; ++i)
            array_free_iterator(array_iters[i]);
        pfree(array_iters);

        PG_RETURN_ARRAYTYPE_P(construct_empty_array(JSONBOID));
    }

    jsonb_datums = palloc(sizeof(Datum) * max_array_len);
    for (int i = 0; i < max_array_len; ++i)
    {
        StringInfoData buf;

        initStringInfo(&buf);
        appendStringInfoChar(&buf, '{');

        for (int j = 0; j < arg_arrays_count; ++j)
        {
            char fstr[32];
            Datum itel_datum;
            bool itel_isnull, has_it;

            if (j > 0) appendStringInfoChar(&buf, ',');
            sprintf(fstr, "\"f%d\":", j + 1);
            appendStringInfoString(&buf, fstr);

            has_it = array_iterate(array_iters[j], &itel_datum, &itel_isnull);

            if (!has_it)
            {
                appendStringInfoString(&buf, "null");
                continue;
            }
            if (arg_arrays_ndims[j] == 1)
            {
                if (itel_isnull)
                {
                    appendStringInfoString(&buf, "null");
                    continue;
                }
                else
                {
                    char* str = pgau_datum_to_json_cstring(itel_datum, arg_arrays_eltps[j]);

                    appendStringInfoString(&buf, str);
                    pfree(str);
                    continue;
                }
            }
            else 
            {
                StringInfoData element_json_str;
                Datum* element_elems;
                bool *element_nulls;
                int element_ndim, element_nelems;
                int* element_dims;
                int16 el_typlen;
                bool el_typbyval;
                char el_typalign;

                ArrayType* element_array = DatumGetArrayTypeP(itel_datum);

                get_typlenbyvalalign(arg_arrays_eltps[j], &el_typlen, &el_typbyval, &el_typalign);
                deconstruct_array(
                    element_array, 
                    arg_arrays_eltps[j], el_typlen, el_typbyval, el_typalign, 
                    &element_elems, 
                    &element_nulls, 
                    &element_nelems
                );
                element_ndim = arg_arrays_ndims[j] - 1;
                element_dims = ARR_DIMS(element_array);

                initStringInfo(&element_json_str);
                pgau_deconstructed_ArrayType_to_StringInfo(
                    &element_json_str, 
                    element_elems, 
                    element_nulls, 
                    arg_arrays_eltps[j], 
                    element_nelems,
                    element_ndim, 
                    element_dims
                );
                appendStringInfoString(&buf, element_json_str.data);
                pfree(element_json_str.data);
                pfree(element_elems);
                pfree(element_nulls);
                pfree(element_array);
            }
        }
        appendStringInfoChar(&buf, '}');
        jsonb_datums[i] = DirectFunctionCall1(jsonb_in, CStringGetDatum(buf.data));
        pfree(buf.data);
    }
    get_typlenbyvalalign(JSONBOID, &res_typlen, &res_typbyval, &res_typalign);

    res_dims[0] = max_array_len;
    res_lbs[0] = 1;
    res_array = construct_md_array(jsonb_datums, NULL, 1, res_dims, res_lbs, JSONBOID, res_typlen, res_typbyval, res_typalign);

    pfree(arg_arrays);
    pfree(arg_arrays_eltps);
    pfree(arg_arrays_ndims);
    for (int i = 0; i < max_array_len; ++i)
        pfree(DatumGetPointer(jsonb_datums[i]));
    pfree(jsonb_datums);
    for (int i = 0; i < arg_arrays_count; ++i)
        array_free_iterator(array_iters[i]);
    pfree(array_iters);

    PG_RETURN_ARRAYTYPE_P(res_array);
}

