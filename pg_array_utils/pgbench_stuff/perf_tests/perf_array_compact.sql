-- 4) array_compact: вход 1M, выход 666667
SELECT array_compact(
    ARRAY(SELECT CASE WHEN i % 3 = 0 THEN 0 ELSE i END
          FROM generate_series(1, 1000000) AS i),
    'zero'
);