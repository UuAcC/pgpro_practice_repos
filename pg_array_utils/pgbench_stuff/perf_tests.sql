-- 1) array_flatten: вход 100k х 10, выход 1M
SELECT array_flatten(
    (SELECT array_agg(
                ARRAY(SELECT i*10 + j FROM generate_series(1, 10) j)
                ORDER BY i)
     FROM generate_series(0, 99999) i)
);

-- 2) array_chunk: вход 1M, выход 200 х 5k
SELECT array_chunk(ARRAY(SELECT generate_series(1, 1000000)), 5000);

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

-- 4) array_compact: вход 1M, выход 666667
SELECT array_compact(
    ARRAY(SELECT CASE WHEN i % 3 = 0 THEN 0 ELSE i END
          FROM generate_series(1, 1000000) AS i),
    'zero'
);

-- 5) array_pad: вход 3, выход 1M
SELECT array_pad(
    ARRAY(SELECT (random()*100)::int FROM generate_series(1, 3)),
    1000000,
    0
);

-- 6) array_slice: вход 1M, выход 90k
SELECT array_slice(
    ARRAY(SELECT generate_series(1, 1000000)),
    1,
    900000,
    10
);

-- 7) array_repeat: вход 100, выход 1M
SELECT array_repeat(
    ARRAY(SELECT generate_series(1, 100)),
    10000
);
