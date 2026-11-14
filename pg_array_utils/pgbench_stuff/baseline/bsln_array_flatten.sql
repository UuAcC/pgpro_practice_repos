-- В подобных файлах размещен код, аналогичный построению массивов для тестов в perf_tests
-- Нужен для вычитания времени на построение массивов и вывод.

-- 1) array_flatten: вход 100k х 10
SELECT length((SELECT array_agg(ARRAY(SELECT i*10 + j FROM generate_series(1, 10) j) ORDER BY i)
               FROM generate_series(0, 99999) i)::text);