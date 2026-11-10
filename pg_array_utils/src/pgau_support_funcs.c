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
You can specify the function used to convert (OUTFUNC). Use InvalidOid value to auto-select.
*/
char*
pgau_datum_to_json_cstring(Datum value, Oid type, Oid outfunc)
{
    Oid     otf;
    bool    typisvarlena;
    char   *result;

    if (outfunc == InvalidOid)
    {
        getTypeOutputInfo(type, &otf, &typisvarlena);
    }
    else otf = outfunc;

    if (type == BOOLOID)
    {
        result = pstrdup(DatumGetBool(value) ? "true" : "false");
    }
    else if (type == INT2OID || type == INT4OID || type == INT8OID ||
        type == FLOAT4OID || type == FLOAT8OID ||
        type == NUMERICOID)
    {
        result = OidOutputFunctionCall(otf, value);
    }
    else
    {
        StringInfoData buf;
        char *raw = OidOutputFunctionCall(otf, value);
        
        initStringInfo(&buf);
        escape_json(&buf, raw);
        result = buf.data;
        pfree(raw);
    }
    return result;
}

/*
Converts anyarray to StringInfo.
Requires arrays of elements (ELEMS) and nulls (NULLS), the type (ELTP) of the anyarray element, number of elements (NELEMS) and dimensions (NDIM) and the pointer to the array of dimensions sizes (DIMS). 
You can specify the function used to convert elements of the array (OUTFUNC). Use InvalidOid value to auto-select.
*/
void
pgau_deconstructed_ArrayType_to_StringInfo(StringInfo dest, Datum* elems, bool* nulls, Oid eltp, int nelems, int ndim, int* dims, Oid outfunc)
{
    int* cnts;
    Oid otf;
    bool typisvarlena;
    
    if (nelems <= 0)
    {
        appendStringInfoString(dest, "[]");
        return;
    }

    if (outfunc == InvalidOid) getTypeOutputInfo(eltp, &otf, &typisvarlena);
    else otf = outfunc;
    
    cnts = palloc0(sizeof(int) * ndim);

    appendStringInfoNChars(dest, '[', ndim);
    for (int i = 0; i < nelems - 1; ++i) 
    {
        int cnt_fbr = 0;

        if (nulls != NULL && nulls[i]) appendStringInfoString(dest, "null");
        else 
        {
            char* str = pgau_datum_to_json_cstring(elems[i], eltp, otf);

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
        char* last = pgau_datum_to_json_cstring(elems[nelems - 1], eltp, otf);

        appendStringInfoString(dest, last);
        pfree(last);
    }
    appendStringInfoNChars(dest, ']', ndim);
    pfree(cnts);
}

/*
Converts anyarray to StringInfo.
Requires the ArrayType*. 
You can specify the function used to convert elements of the array (OUTFUNC). Use InvalidOid value to auto-select.
*/
void
pgau_ArrayType_to_StringInfo(StringInfo dest, ArrayType* array, Oid outfunc)
{
    int* cnts, ndim, *dims, nelems;
    Oid eltp;
    Datum itel_datum;
    bool itel_isnull;
    ArrayIterator iter;
    Oid otf;
    bool typisvarlena;

    ndim = ARR_NDIM(array);
    dims = ARR_DIMS(array);
    nelems = ArrayGetNItems(ndim, dims);
    eltp = ARR_ELEMTYPE(array);

    if (nelems <= 0)
    {
        appendStringInfoString(dest, "[]");
        return;
    }

    if (outfunc == InvalidOid) getTypeOutputInfo(eltp, &otf, &typisvarlena);
    else otf = outfunc;

    cnts = palloc0(sizeof(int) * ndim);
    iter = array_create_iterator(array, 0, NULL);

    appendStringInfoNChars(dest, '[', ndim);
    for (int i = 0; i < nelems - 1; ++i) 
    {
        int cnt_fbr;

        cnt_fbr = 0;
        array_iterate(iter, &itel_datum, &itel_isnull);
        if (itel_isnull) appendStringInfoString(dest, "null");
        else 
        {
            char* str = pgau_datum_to_json_cstring(itel_datum, eltp, otf);

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
        char* last = pgau_datum_to_json_cstring(itel_datum, eltp, otf);

        appendStringInfoString(dest, last);
        pfree(last);
    }
    appendStringInfoNChars(dest, ']', ndim);
    array_free_iterator(iter);
    pfree(cnts);
}

