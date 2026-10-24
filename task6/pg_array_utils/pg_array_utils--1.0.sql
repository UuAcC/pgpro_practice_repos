CREATE OR REPLACE FUNCTION array_flatten(anyarray)
RETURNS anyarray 
AS 'MODULE_PATHNAME', 'array_flatten'
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION array_chunk(anyarray, _size integer)
RETURNS anyarray 
AS 'MODULE_PATHNAME', 'array_chunk'
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION array_zip(VARIADIC "any") 
RETURNS jsonb[] 
AS 'MODULE_PATHNAME', 'array_zip'
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION array_compact(anyarray, _mode text DEFAULT 'null')
RETURNS anyarray 
AS 'MODULE_PATHNAME', 'array_compact'
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION array_pad(anyarray, _size integer, _value anynonarray)
RETURNS anyarray 
AS 'MODULE_PATHNAME', 'array_pad'
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION array_pad(anyarray, _size integer, _value anyarray)
RETURNS anyarray 
AS 'MODULE_PATHNAME', 'array_pad_arr'
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION array_slice(anyarray, _start integer, _end integer, _step integer DEFAULT 1)
RETURNS anyarray 
AS 'MODULE_PATHNAME', 'array_slice'
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION array_repeat(anynonarray, _count integer)
RETURNS anyarray 
AS 'MODULE_PATHNAME', 'array_repeat'
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION array_repeat(anyarray, _count integer)
RETURNS anyarray 
AS 'MODULE_PATHNAME', 'array_repeat_arr'
LANGUAGE C IMMUTABLE STRICT;

