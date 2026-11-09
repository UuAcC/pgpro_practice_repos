#include "pg_array_utils.h"

/*
Compares the value (A) of the type (ELTP) which represents a number from a human perspective (int, float, etc.) to zero.
*/
bool
pgau_numeric_is_zero(Oid eltp, Datum a) 
{
    switch(eltp)
    {
        case INT2OID: { int16 x = DatumGetInt16(a); return x == (int16)0; }
        case INT4OID: { int32 x = DatumGetInt32(a); return x == (int32)0; } 
        case INT8OID:
        case MONEYOID: { int64 x = DatumGetInt64(a); return x == (int64)0; }
        case OIDOID: { uint32 x = DatumGetObjectId(a); return x == (uint32)0; }
        case FLOAT4OID: { return float4_cmp_internal(DatumGetFloat4(a), (float4)0.0) == 0; }
        case FLOAT8OID: { return float8_cmp_internal(DatumGetFloat8(a), (float8)0.0) == 0; }
        case NUMERICOID: 
        { 
            Datum numeric_zero = DirectFunctionCall1(int4_numeric, Int32GetDatum(0));
            return DatumGetInt32(DirectFunctionCall2(numeric_cmp, a, numeric_zero)) == 0; 
        }
        default: 
        { 
            ereport(ERROR,
                    (errcode(ERRCODE_DATATYPE_MISMATCH),
                     errmsg("Can't compare %s to zero", format_type_be(eltp)))); 
        }
    }
}

/*
Compares the value (A) of the type (ELTP) which represents a text from a human perspective (text, varchar, bytea, etc.) to an empty string.
*/
bool
pgau_text_is_empty(Oid eltp, Datum a)
{
    switch(eltp)
    {
        case TEXTOID: 
        case VARCHAROID:
        case BPCHAROID:
        {
            char *str = TextDatumGetCString(a);
            bool res = (str[0] == '\0');
            pfree(str);
            return res;
        }
        case CHAROID: { return DatumGetChar(a) == '\0'; }
        case NAMEOID: { return NameStr(*DatumGetName(a))[0] == '\0'; }
        case BYTEAOID: { return VARSIZE_ANY_EXHDR(DatumGetByteaPP(a)) == 0; }
        default:
        {
            ereport(ERROR,
                    (errcode(ERRCODE_DATATYPE_MISMATCH),
                     errmsg("Can't compare %s to empty string (\"\")", format_type_be(eltp)))); 
        }
    }
}

/*
Converts the value (A) of the type (TYPE) to char* in json format.
*/
char*
pgau_datum_to_json_cstring(Datum value, Oid type)
{
    Oid     outfunc;
    bool    typisvarlena;
    char   *result;

    getTypeOutputInfo(type, &outfunc, &typisvarlena);

    if (type == BOOLOID)
    {
        result = pstrdup(DatumGetBool(value) ? "true" : "false");
    }
    else if (type == INT2OID || type == INT4OID || type == INT8OID ||
        type == FLOAT4OID || type == FLOAT8OID ||
        type == NUMERICOID)
    {
        result = OidOutputFunctionCall(outfunc, value);
    }
    else
    {
        StringInfoData buf;
        char *raw = OidOutputFunctionCall(outfunc, value);
        
        initStringInfo(&buf);
        escape_json(&buf, raw);
        result = buf.data;
        pfree(raw);
    }
    return result;
}

/*
Converts anyarray to StringInfo. 
Requires arrays of elements (ELEMS) and nulls (NULLS), the type (ELTP) of the anyarray element, number of elements 
(NELEMS) and dimensions (NDIM) and the pointer to the array of dimensions sizes (DIMS).
*/
void
pgau_deconstructed_ArrayType_to_StringInfo(StringInfo dest, Datum* elems, bool* nulls, Oid eltp, int nelems, int ndim, int* dims)
{
    int* cnts;
    
    if (nelems <= 0)
    {
        appendStringInfoString(dest, "[]");
        return;
    }
    appendStringInfoNChars(dest, '[', ndim);
    cnts = palloc0(sizeof(int) * ndim);
    for (int i = 0; i < nelems - 1; ++i) 
    {
        int cnt_fbr = 0;

        if (nulls != NULL && nulls[i]) appendStringInfoString(dest, "null");
        else 
        {
            char* str = pgau_datum_to_json_cstring(elems[i], eltp);

            appendStringInfoString(dest, str);
            pfree(str);
        }
        for (int j = 0; j < ndim; ++j) 
        {
            cnts[j]++;
            if (cnts[j] == dims[ndim-1 - j]) 
            {
                cnts[j] = 0;
                cnt_fbr++;
            }
            else break;
        }
        appendStringInfoNChars(dest, ']', cnt_fbr);
        appendStringInfoChar(dest, ',');
        appendStringInfoNChars(dest, '[', cnt_fbr);
    }
    if (nulls != NULL && nulls[nelems - 1]) appendStringInfoString(dest, "null");
    else 
    {
        char* last = pgau_datum_to_json_cstring(elems[nelems - 1], eltp);

        appendStringInfoString(dest, last);
        pfree(last);
    }
    appendStringInfoNChars(dest, ']', ndim);
    pfree(cnts);
}

/*
Converts anyarray to StringInfo. 
Requires only the ArrayType*.
*/
void
pgau_ArrayType_to_StringInfo(StringInfo dest, ArrayType* array)
{
    int* cnts, ndim, *dims, nelems;
    Oid eltp;
    Datum itel_datum;
    bool itel_isnull;
    ArrayIterator iter;

    ndim = ARR_NDIM(array);
    dims = ARR_DIMS(array);
    nelems = ArrayGetNItems(ndim, dims);
    eltp = ARR_ELEMTYPE(array);

    if (nelems <= 0)
    {
        appendStringInfoString(dest, "[]");
        return;
    }
    appendStringInfoNChars(dest, '[', ndim);
    cnts = palloc0(sizeof(int) * ndim);
    iter = array_create_iterator(array, 0, NULL);
    for (int i = 0; i < nelems - 1; ++i) 
    {
        int cnt_fbr;

        cnt_fbr = 0;
        array_iterate(iter, &itel_datum, &itel_isnull);
        if (itel_isnull) appendStringInfoString(dest, "null");
        else 
        {
            char* str = pgau_datum_to_json_cstring(itel_datum, eltp);

            appendStringInfoString(dest, str);
            pfree(str);
        }
        for (int j = 0; j < ndim; ++j) 
        {
            cnts[j]++;
            if (cnts[j] == dims[ndim-1 - j]) 
            {
                cnts[j] = 0;
                cnt_fbr++;
            }
            else break;
        }
        appendStringInfoNChars(dest, ']', cnt_fbr);
        appendStringInfoChar(dest, ',');
        appendStringInfoNChars(dest, '[', cnt_fbr);
    }
    array_iterate(iter, &itel_datum, &itel_isnull);
    if (itel_isnull) appendStringInfoString(dest, "null");
    else 
    {
        char* last = pgau_datum_to_json_cstring(itel_datum, eltp);

        appendStringInfoString(dest, last);
        pfree(last);
    }
    appendStringInfoNChars(dest, ']', ndim);
    array_free_iterator(iter);
    pfree(cnts);
}

