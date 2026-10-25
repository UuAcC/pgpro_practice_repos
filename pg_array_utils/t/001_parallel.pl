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

$node->safe_psql('postgres', 'CREATE TABLE parallel_results (id int, result text);');

my $s1 = $node->background_psql('postgres');
my $s2 = $node->background_psql('postgres');
my $s3 = $node->background_psql('postgres');
my $s4 = $node->background_psql('postgres');

# 1. request: flatten on 1
$s1->query_safe(
    "INSERT INTO parallel_results SELECT 1, array_flatten(ARRAY[[1,2],[3,4]])::text;");
# 2. request: chunk on 2
$s2->query_safe(
    "INSERT INTO parallel_results SELECT 2, array_chunk(ARRAY[1,2,3,4], 2)::text;");
# 3. request: zip on 3
$s3->query_safe(
    "INSERT INTO parallel_results SELECT 3, array_zip(ARRAY[1,2,3], ARRAY['a','b','c'])::text;");
# 4. request: compact on 2
$s2->query_safe(
    "INSERT INTO parallel_results SELECT 4, array_compact(ARRAY[0,8,800,5,0,0,5,5,0,0,0,0,35,35], 'zero')::text;");
# 5. request: pad on 3
$s3->query_safe(
    "INSERT INTO parallel_results SELECT 5, array_pad(ARRAY[[1,2],[3,4]], -3, ARRAY[0,0])::text;");
# 6. request: slice on 4
$s4->query_safe(
    "INSERT INTO parallel_results SELECT 6, array_slice(ARRAY[1,2,3,4,5], 2, 4)::text;");
# 7. request: repeat on 1
$s1->query_safe(
    "INSERT INTO parallel_results SELECT 7, array_repeat(7, 3)::text;");

$s1->quit;
$s2->quit;
$s3->quit;
$s4->quit;

# 1. check: flatten on 1
is($node->safe_psql('postgres', 
        "SELECT result FROM parallel_results WHERE id = 1;"),
        '{1,2,3,4}',
        'parallel session 1 - array_flatten');
# 2. check: chunk on 2
is($node->safe_psql('postgres', 
        "SELECT result FROM parallel_results WHERE id = 2;"),
        '{{1,2},{3,4}}', 
        'parallel session 2 - array_chunk');
# 3. check: zip on 3
is($node->safe_psql('postgres',
        "SELECT result FROM parallel_results WHERE id = 3;"),
        '{"{\"f1\": 1, \"f2\": \"a\"}","{\"f1\": 2, \"f2\": \"b\"}","{\"f1\": 3, \"f2\": \"c\"}"}', 
        'parallel session 3 - array_zip');
# 4. check: compact on 2
is($node->safe_psql('postgres',
        "SELECT result FROM parallel_results WHERE id = 4;"),
        '{8,800,5,5,5,35,35}', 
        'parallel session 2 - array_compact'); 
# 5. check: pad on 3
is($node->safe_psql('postgres',
        "SELECT result FROM parallel_results WHERE id = 5;"),
        '{{0,0},{1,2},{3,4}}', 
        'parallel session 3 - array_pad'); 
# 6. check: slice on 4
is($node->safe_psql('postgres',
        "SELECT result FROM parallel_results WHERE id = 6;"),
        '{2,3,4}', 
        'parallel session 4 - array_slice');
# 7. check: repeat on 1
is($node->safe_psql('postgres',
        "SELECT result FROM parallel_results WHERE id = 7;"),
        '{7,7,7}', 
        'parallel session 1 - array_repeat');

$node->stop;
