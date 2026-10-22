#include "pg_array_utils.h"

/*
Checks does the type represent a number from a human perspective (int, float, etc.)
*/
const bool 
pgau_is_numeric_type(Oid eltp)
{
    switch(eltp)
    {
        case INT2OID:
        case INT4OID:
        case INT8OID:
        case FLOAT4OID:
        case FLOAT8OID:
        case NUMERICOID:
        case OIDOID:
        case MONEYOID: return true;
        default: return false;
    }
}

/*
Checks does the type represent a text from a human perspective (text, varchar, bytea, etc.)
*/
const bool 
pgau_is_text_type(Oid eltp)
{
    switch(eltp)
    {
        case TEXTOID:
        case VARCHAROID:
        case BPCHAROID:
        case CHAROID:
        case NAMEOID:
        case BYTEAOID: return true;
        default: return false;
    }
}

/*
Compares the value (A) of the type (ELTP) which represents a number from a human perspective (int, float, etc.) to zero
*/
const bool
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
            elog(ERROR, "Can't compare %s to zero", format_type_be(eltp)); 
        }
    }
}

/*
Compares the value (A) of the type (ELTP) which represents a text from a human perspective (text, varchar, bytea, etc.) to an empty string
*/
const bool
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
            elog(ERROR, "Can't compare %s to empty string (\'\')", format_type_be(eltp)); 
        }
    }
}

const bool 
pgau_array_is_empty(int ndim, int* dims)
{
    return ndim == 0 || ArrayGetNItems(ndim, dims) == 0;
}

/*
Appends N chars C into StringInfo STR
*/
void
appendStringInfoNChars(StringInfo str, char c, int n)
{
    if (n <= 0) return;
    enlargeStringInfo(str, n);
    memset(str->data + str->len, c, n);
    str->len += n;
    str->data[str->len] = '\0';
}

char*
pgau_datum_to_json_cstring(Datum value, Oid type)
{
    Oid     outfunc;
    bool    typisvarlena;
    char   *result;

    getTypeOutputInfo(type, &outfunc, &typisvarlena);

    if (type == INT2OID || type == INT4OID || type == INT8OID ||
        type == FLOAT4OID || type == FLOAT8OID ||
        type == NUMERICOID || type == BOOLOID)
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

void
pgau_array_to_StringInfo(StringInfo dest, Datum* elems, bool* nulls, Oid eltp, int nelems, int ndim, int* dims)
{
    if (nelems <= 0)
    {
        appendStringInfoString(dest, "{}");
        return;
    }
    appendStringInfoNChars(dest, '{', ndim);
    int* cnts = palloc0(sizeof(int) * ndim);
    for (int i = 0; i < nelems - 1; ++i) 
    {
        int cnt_fbr = 0;
        if (nulls != NULL && nulls[i]) appendStringInfoString(dest, "NULL");
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
        appendStringInfoNChars(dest, '}', cnt_fbr);
        appendStringInfoChar(dest, ',');
        appendStringInfoNChars(dest, '{', cnt_fbr);
    }
    char* last = pgau_datum_to_json_cstring(elems[nelems - 1], eltp);
    appendStringInfoString(dest, last);
    pfree(last);
    appendStringInfoNChars(dest, '}', ndim);
    pfree(cnts);
}

/*

{{{1,2,3},{4,5,6}},{{7,8,9},{10,11,12}}}
   0 1 2   3 4 5     6 7 8   9  10 11

stroka += '{' * ndim;
int* cnts = palloc0(sizeof(int) * ndim);
for (int i = 0; i < nelems - 1; ++i) {
    stroka += inner_array[i];
    cnt_fbr = 0;
    for (int j = 0; j < ndim; ++j) {
        cnts[j]++;
        if (cnts[j] == dims[ndim-1 - j]) {
            cnts[j] = 0;
            cnt_fbr++;
        }
        else break;
    }
    stroka += '}' * cnt_fbr + ',' + '{' * cnt_fbr;
}
stroka += inner_array[nelems - 1];
stroka += '}' * ndim;

*/

