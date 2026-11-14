-- 6) array_slice: вход 1M, выход 90k
SELECT array_slice(
    ARRAY(SELECT generate_series(1, 1000000)),
    1,
    900000,
    10
);