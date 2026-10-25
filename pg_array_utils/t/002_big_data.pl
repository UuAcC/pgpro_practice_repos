#!/usr/bin/perl
use strict;
use warnings;

use PostgreSQL::Test::Cluster;
use PostgreSQL::Test::Utils;
use Test::More;

plan tests => 7;

my $node = PostgreSQL::Test::Cluster->new('main');
$node->init;
$node->start;

$node->safe_psql('postgres', 'CREATE EXTENSION pg_array_utils;');

# 1. flatten
my $r = $node->safe_psql('postgres',
        "SELECT array_length(array_flatten(array_fill(1, ARRAY[200, 5000])), 1);");
is($r, '1000000', 'array_flatten on 200x5000 array');

# 2. chunk
$r = $node->safe_psql('postgres',
        "SELECT array_dims(array_chunk(ARRAY(SELECT generate_series(1, 1000000)), 500));");
is($r, '[1:2000][1:500]', 'array_chunk on 100000-element array');

# 3. zip
$r = $node->safe_psql('postgres',
        "SELECT array_length(array_zip("
    .   "ARRAY(SELECT generate_series(1, 100000)),"
    .   "ARRAY(SELECT generate_series(1, 100000)),"
    .   "ARRAY(SELECT generate_series(1, 100000))), 1);");
is($r, '100000', 'array_zip on three 100000-element arrays');

# 4. compact
$r = $node->safe_psql('postgres',
        "SELECT array_length(array_compact("
    .   "ARRAY(SELECT CASE WHEN i % 3 = 0 THEN 0 ELSE i END "
    .   "FROM generate_series(1, 100000) AS i), 'zero'), 1);");
is($r, '66667', 'array_compact on 100000-element array');

# 5. pad
$r = $node->safe_psql('postgres',
        "SELECT array_length(array_pad(ARRAY[1,2,3], 100000, 0), 1);");
is($r, '100000', 'array_pad on 99997-element padding');

# 6. slice
$r = $node->safe_psql('postgres',
        "SELECT array_length(array_slice(ARRAY(SELECT generate_series(1, 100000)), 1, 100000, 2), 1);");
is($r, '50000', 'array_slice on 100000-element array with step=2');

# 7. repeat
$r = $node->safe_psql('postgres',
    "SELECT array_length(array_repeat(0, 10000), 1);");
is($r, '10000', 'array_repeat with large count');

$node->stop;
