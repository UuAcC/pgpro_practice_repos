-- 1) array_flatten: вход 100k х 10, выход 1M
SELECT array_flatten(
    (SELECT array_agg(
                ARRAY(SELECT i*10 + j FROM generate_series(1, 10) j)
                ORDER BY i)
     FROM generate_series(0, 99999) i)
);