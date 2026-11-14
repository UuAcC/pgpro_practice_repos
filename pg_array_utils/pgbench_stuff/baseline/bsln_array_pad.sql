-- В подобных файлах размещен код, аналогичный построению массивов для тестов в perf_tests
-- Нужен для вычитания времени на построение массивов и вывод.

-- 5) array_pad: вход 3 
SELECT length(ARRAY(SELECT (random()*100)::int FROM generate_series(1, 3))::text);