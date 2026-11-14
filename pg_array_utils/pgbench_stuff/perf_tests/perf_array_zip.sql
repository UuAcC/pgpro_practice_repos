-- 3) array_zip: вход — 10 х 100k 
SELECT array_zip(
    ARRAY(SELECT generate_series(1, 100000)),
    ARRAY(SELECT generate_series(1, 100000)),
    ARRAY(SELECT generate_series(1, 100000)),
    ARRAY(SELECT generate_series(1, 100000)),
    ARRAY(SELECT generate_series(1, 100000)),
    ARRAY(SELECT generate_series(1, 100000)),
    ARRAY(SELECT generate_series(1, 100000)),
    ARRAY(SELECT generate_series(1, 100000)),
    ARRAY(SELECT generate_series(1, 100000)),
    ARRAY(SELECT generate_series(1, 100000))
);