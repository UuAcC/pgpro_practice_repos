-- В подобных файлах размещен код, аналогичный построению массивов для тестов в perf_tests
-- Нужен для вычитания времени на построение массивов и вывод.

-- 4) array_compact: вход 1M
SELECT length(ARRAY(SELECT CASE WHEN i % 3 = 0 THEN 0 ELSE i END
                      FROM generate_series(1, 1000000) AS i)::text);