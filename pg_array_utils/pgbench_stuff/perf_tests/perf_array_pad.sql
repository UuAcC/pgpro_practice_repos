-- 5) array_pad: вход 3, выход 1M
SELECT array_pad(
    ARRAY(SELECT (random()*100)::int FROM generate_series(1, 3)),
    1000000,
    0
);