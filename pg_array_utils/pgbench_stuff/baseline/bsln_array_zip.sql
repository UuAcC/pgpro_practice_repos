-- В подобных файлах размещен код, аналогичный построению массивов для тестов в perf_tests
-- Нужен для вычитания времени на построение массивов и вывод.

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