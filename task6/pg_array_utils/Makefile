MODULE_big = pg_array_utils
OBJS = \
    pg_array_utils.o \
    pgau_array_chunk.o \
    pgau_array_compact.o \
    pgau_array_flatten.o \
    pgau_array_pad.o \
    pgau_array_repeat.o \
    pgau_array_slice.o \
    pgau_array_zip.o \
    pgau_support_funcs.o

EXTENSION = pg_array_utils
DATA = pg_array_utils--1.0.sql

PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
