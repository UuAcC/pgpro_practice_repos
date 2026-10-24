-- -----------------------------------------------------------
--                     Обычные случаи:
-- -----------------------------------------------------------

-- 1D массив, size=2
SELECT array_chunk(ARRAY[1,2,3,4,5], 2);
-- Ожидается: {{1,2},{3,4},{5,NULL}}

-- size > n: один чанк с NULL-дополнением
SELECT array_chunk(ARRAY['good luck','have fun'], 5);
-- Ожидается: {{"good luck","have fun",NULL,NULL,NULL}}

-- Массив с NULL-элементами
SELECT array_chunk(ARRAY[1,NULL,3], 2);
-- Ожидается: {{1,NULL},{3,NULL}}

-- Большой 1D массив integer (производительность)
SELECT array_chunk(ARRAY(SELECT generate_series(1,10000)), 500);

-- -----------------------------------------------------------
--                    Обработка ошибок:
-- -----------------------------------------------------------

-- N-мерный массив
SELECT array_chunk(ARRAY[[1,2],[3,4]], 2);
-- Ошибка: The argument array should be one-dimensional

-- Пустой массив
SELECT array_chunk(ARRAY[]::int[], 2);
-- Ошибка: The argument array can't be empty

-- size <= 0
SELECT array_chunk(ARRAY[1,2,3], 0);
-- Ошибка: The chunk size should be greater than zero

-- 1-й аргумент - не массив
SELECT array_chunk(5431, 2);
-- Ошибка: function array_chunk(integer, integer) does not exist

-- size - не integer
SELECT array_chunk(ARRAY[5,4,3,1], 'broken');
-- Ошибка приведения типов (invalid input syntax for type integer)

