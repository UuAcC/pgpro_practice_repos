#include "postgres.h"
#include "fmgr.h"
#include "utils/builtins.h"

PG_MODULE_MAGIC;

PG_FUNCTION_INFO_V1(hello_world);

Datum 
hello_world(PG_FUNCTION_ARGS) {
	text* arg_text = PG_GETARG_TEXT_PP(0);
    char* c_string = text_to_cstring(arg_text);
    elog(NOTICE, "Hello world! Your input: %s", c_string);
    pfree(c_string);

    PG_RETURN_VOID();
}
