-- 7) array_repeat: вход 100, выход 1M
SELECT array_repeat(
    ARRAY(SELECT generate_series(1, 100)),
    10000
);