-- 2) array_chunk: вход 1M, выход 200 х 5k
SELECT array_chunk(ARRAY(SELECT generate_series(1, 1000000)), 5000);