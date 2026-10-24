MODULE_big = pg_array_utils
EXTENSION = pg_array_utils
DATA = pg_array_utils--1.0.sql

OBJS = $(patsubst %.c,%.o,$(wildcard src/*.c))

PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
