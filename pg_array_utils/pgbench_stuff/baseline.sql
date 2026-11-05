-- Здесь размещен код, аналогичный построению массивов для тестов в perf_tests.sql
-- Нужен для вычитания времени на построение массивов и вывод.

-- 1) array_flatten: вход 100k х 10
SELECT length((SELECT array_agg(ARRAY(SELECT i*10 + j FROM generate_series(1, 10) j) ORDER BY i)
               FROM generate_series(0, 99999) i)::text);

-- 2) array_chunk: вход 1M
SELECT length(ARRAY(SELECT generate_series(1, 1000000))::text);

-- 3) array_zip: вход — 10 х 100k 
SELECT
    array_length(ARRAY(SELECT generate_series(1, 100000)), 1)
  + array_length(ARRAY(SELECT generate_series(1, 100000)), 1)
  + array_length(ARRAY(SELECT generate_series(1, 100000)), 1)
  + array_length(ARRAY(SELECT generate_series(1, 100000)), 1)
  + array_length(ARRAY(SELECT generate_series(1, 100000)), 1)
  + array_length(ARRAY(SELECT generate_series(1, 100000)), 1)
  + array_length(ARRAY(SELECT generate_series(1, 100000)), 1)
  + array_length(ARRAY(SELECT generate_series(1, 100000)), 1)
  + array_length(ARRAY(SELECT generate_series(1, 100000)), 1)
  + array_length(ARRAY(SELECT generate_series(1, 100000)), 1);

-- 4) array_compact: вход 1M
SELECT length(ARRAY(SELECT CASE WHEN i % 3 = 0 THEN 0 ELSE i END
                      FROM generate_series(1, 1000000) AS i)::text);

-- 5) array_pad: вход 3 
SELECT length(ARRAY(SELECT (random()*100)::int FROM generate_series(1, 3))::text);

-- 6) array_slice: вход 1M
SELECT length(ARRAY(SELECT generate_series(1, 1000000))::text);

-- 7) array_repeat: вход 100
SELECT length(ARRAY(SELECT generate_series(1, 100))::text);
