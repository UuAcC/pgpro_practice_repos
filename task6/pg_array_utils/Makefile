MODULE_big = pg_array_utils
EXTENSION = pg_array_utils
DATA = pg_array_utils--1.0.sql

OBJS = $(patsubst %.c,%.o,$(wildcard src/*.c))
REGRESS = \
	init__ \
	array_flatten_tests \
	array_chunk_tests \
	array_zip_tests \
	array_compact_tests \
	array_pad_tests \
	array_slice_tests \
	array_repeat_tests \
	destruct__

PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
