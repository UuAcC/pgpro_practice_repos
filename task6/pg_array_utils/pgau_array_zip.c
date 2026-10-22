#include "pg_array_utils.h"

Datum
array_zip(PG_FUNCTION_ARGS)
{
    ArrayType** arg_arrays, *res_array;
    Datum** arg_arrays_elements, *jsonb_datums;
    bool** arg_arrays_nulls;
    int* arg_arrays_nelems;

    int *arg_arrays_element_nelems;

    Oid* arg_arrays_eltps;
    int16* arg_arrays_typlens;
    bool* arg_arrays_typbyvals, *arg_arrays_empty;
    char* arg_arrays_typaligns;

    int* arg_arrays_ndims;
    int** arg_arrays_dims;
    int max_array_len;
    int arg_arrays_count = PG_NARGS();

    if (arg_arrays_count == 0)
    {
        elog(ERROR, "array_zip requires at least one array");
    }

    arg_arrays = palloc(sizeof(ArrayType*) * arg_arrays_count);
    arg_arrays_elements = palloc(sizeof(Datum*) * arg_arrays_count);
    arg_arrays_nulls = palloc(sizeof(bool*) * arg_arrays_count);
    arg_arrays_nelems = palloc(sizeof(int) * arg_arrays_count);
    arg_arrays_empty = palloc(sizeof(bool) * arg_arrays_count);

    arg_arrays_element_nelems = palloc0(sizeof(int) * arg_arrays_count);

    arg_arrays_eltps = palloc(sizeof(Oid) * arg_arrays_count);
    arg_arrays_typlens = palloc(sizeof(int16) * arg_arrays_count);
    arg_arrays_typbyvals = palloc(sizeof(bool) * arg_arrays_count);
    arg_arrays_typaligns = palloc(sizeof(char) * arg_arrays_count);

    arg_arrays_ndims = palloc(sizeof(int) * arg_arrays_count);
    arg_arrays_dims = palloc(sizeof(int*) * arg_arrays_count);

    max_array_len = 0;

    for (int i = 0; i < arg_arrays_count; ++i)
    {
        arg_arrays[i] = PG_GETARG_ARRAYTYPE_P(i);

        arg_arrays_eltps[i] = ARR_ELEMTYPE(arg_arrays[i]);
        get_typlenbyvalalign(
            arg_arrays_eltps[i], 
            &arg_arrays_typlens[i], 
            &arg_arrays_typbyvals[i], 
            &arg_arrays_typaligns[i]
        );
        arg_arrays_ndims[i] = ARR_NDIM(arg_arrays[i]);
        arg_arrays_dims[i] = ARR_DIMS(arg_arrays[i]);
        
        arg_arrays_empty[i] = pgau_array_is_empty(arg_arrays_ndims[i], arg_arrays_dims[i]);
        max_array_len = (!arg_arrays_empty[i] && (arg_arrays_dims[i][0] > max_array_len)) ? arg_arrays_dims[i][0] : max_array_len;

        deconstruct_array(
            arg_arrays[i], 
            arg_arrays_eltps[i], 
            arg_arrays_typlens[i], 
            arg_arrays_typbyvals[i],
            arg_arrays_typaligns[i],
            &arg_arrays_elements[i],
            &arg_arrays_nulls[i],
            &arg_arrays_nelems[i]
        );
        arg_arrays_element_nelems[i] = (!arg_arrays_empty[i]) ? arg_arrays_nelems[i] / arg_arrays_dims[i][0] : 0;
    }

    if (max_array_len == 0)
    {
        pfree(arg_arrays);
        for (int i = 0; i < arg_arrays_count; ++i)
        {
            if (arg_arrays_elements[i]) pfree(arg_arrays_elements[i]);
            if (arg_arrays_nulls[i]) pfree(arg_arrays_nulls[i]);
        }
        pfree(arg_arrays_nelems);
        pfree(arg_arrays_empty);

        pfree(arg_arrays_element_nelems);

        pfree(arg_arrays_eltps);
        pfree(arg_arrays_typlens);
        pfree(arg_arrays_typbyvals);
        pfree(arg_arrays_typaligns);
        pfree(arg_arrays_ndims);
        pfree(arg_arrays_dims);

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
            if (j > 0) appendStringInfoChar(&buf, ',');
            char fstr[32];
            sprintf(fstr, "\"f%d\":", j + 1);
            appendStringInfoString(&buf, fstr);
            if (arg_arrays_empty[j])
            {
                appendStringInfoString(&buf, "NULL");
                continue;
            }
            if (arg_arrays_ndims[j] == 1)
            {
                if (arg_arrays_nelems[j] <= i || (arg_arrays_nulls[j] != NULL && arg_arrays_nulls[j][i]))
                {
                    appendStringInfoString(&buf, "NULL");
                    continue;
                }
                else
                {
                    char* str = pgau_datum_to_json_cstring(arg_arrays_elements[j][i], arg_arrays_eltps[j]);
                    appendStringInfoString(&buf, str);
                    pfree(str);
                    continue;
                }
            }
            else 
            {
                if (arg_arrays_dims[j][0] <= i)
                {
                    appendStringInfoString(&buf, "NULL");
                    continue;
                }
                else
                {
                    StringInfoData element_json_str;
                    initStringInfo(&element_json_str);
                    Datum* element_elems =  arg_arrays_elements[j] + arg_arrays_element_nelems[j] * i;
                    bool *element_nulls = (arg_arrays_nulls[j]) ? arg_arrays_nulls[j] + arg_arrays_element_nelems[j] * i : NULL;
                    Oid element_eltp =  arg_arrays_eltps[j];
                    int element_ndim = arg_arrays_ndims[j] - 1;
                    int* element_dims = arg_arrays_dims[j] + 1;
                    pgau_array_to_StringInfo(
                        &element_json_str, 
                        element_elems, 
                        element_nulls, 
                        element_eltp, 
                        arg_arrays_element_nelems[j],
                        element_ndim, 
                        element_dims
                    );
                    appendStringInfoString(&buf, element_json_str.data);
                    pfree(element_json_str.data);
                }
            }
        }
        appendStringInfoChar(&buf, '}');
        jsonb_datums[i] = DirectFunctionCall1(jsonb_in, CStringGetDatum(buf.data));
        pfree(buf.data);
    }
    int16 res_typlen;
    bool  res_typbyval;
    char  res_typalign;
    get_typlenbyvalalign(JSONBOID, &res_typlen, &res_typbyval, &res_typalign);

    int res_dims[1] = {max_array_len};
    int res_lbs[1] = {1};
    res_array = construct_md_array(jsonb_datums, NULL, 1, res_dims, res_lbs, JSONBOID, res_typlen, res_typbyval, res_typalign);

    pfree(arg_arrays);
    for (int i = 0; i < arg_arrays_count; ++i)
    {
        if (arg_arrays_elements[i]) pfree(arg_arrays_elements[i]);
        if (arg_arrays_nulls[i]) pfree(arg_arrays_nulls[i]);
    }
    pfree(arg_arrays_nelems);
    pfree(arg_arrays_empty);

    pfree(arg_arrays_element_nelems);

    pfree(arg_arrays_eltps);
    pfree(arg_arrays_typlens);
    pfree(arg_arrays_typbyvals);
    pfree(arg_arrays_typaligns);
    pfree(arg_arrays_ndims);
    pfree(arg_arrays_dims);

    for (int i = 0; i < max_array_len; ++i)
        pfree(DatumGetPointer(jsonb_datums[i]));
    pfree(jsonb_datums);

    PG_RETURN_ARRAYTYPE_P(res_array);
}

