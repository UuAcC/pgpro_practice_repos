-- -----------------------------------------------------------
--                     Обычные случаи:
-- -----------------------------------------------------------

-- Срез с 1 по 3, шаг 1 (по умолчанию)
SELECT array_slice(ARRAY[1,2,3,4,5], 1, 3);
-- Ожидается: {1,2,3}

-- Срез с отрицательными индексами (с конца)
SELECT array_slice(ARRAY[1,2,3,4,5], -3, -1);
-- Ожидается: {3,4,5}

-- Срез с шагом 2
SELECT array_slice(ARRAY[1,2,3,4,5], 1, 5, 2);
-- Ожидается: {1,3,5}

-- Срез с шагом 3
SELECT array_slice(ARRAY[1,2,3,4,5,6,7], 1, 7, 3);
-- Ожидается: {1,4,7}

-- start == end
SELECT array_slice(ARRAY[1,2,3], 2, 2);
-- Ожидается: {2}

-- start == end (отрицательные)
SELECT array_slice(ARRAY[1,2,3,4,5], -1, -1);
-- Ожидается: {5}

-- step >= длины первого измерения
SELECT array_slice(ARRAY[1,2,3], 1, 3, 5);
-- Ожидается: {1}

-- N-мерный массив: срез по первому измерению
SELECT array_slice(ARRAY[[1,2],[3,4],[5,6]], 1, 2);
-- Ожидается: {{1,2},{3,4}}

-- Массив с NULL-элементами
SELECT array_slice(ARRAY[1,NULL,3,4], 2, 4);
-- Ожидается: {NULL,3,4}

-- Текстовый массив
SELECT array_slice(ARRAY['а','б','в','где','ёж'], 4, 5);
-- Ожидается: {где,ёж}

-- Большой массив (производительность)
SELECT array_slice(ARRAY(SELECT generate_series(1,10000)), 100, 9000, 10);

-- -----------------------------------------------------------
--                    Обработка ошибок:
-- -----------------------------------------------------------

-- Пустой массив
SELECT array_slice(ARRAY[]::int[], 1, 1);
-- Ошибка: The argument array can't be empty

-- start > end
SELECT array_slice(ARRAY[1,2,3], 3, 2);
-- Ошибка: The start index should be between 1 and the end index

-- Отрицательный step
SELECT array_slice(ARRAY[1,2,3], 1, 2, -1);
-- Ошибка: The step should be greater than zero

-- end за пределами массива
SELECT array_slice(ARRAY[1,2,3], 1, 5);
-- Ошибка: The end index should be between the start index and the lenght of the first dimension of the array

-- start = 0 (строго 1-based)
SELECT array_slice(ARRAY[1,2,3], 0, 2);
-- Ошибка: The start index should be between 1 and the end index

-- 1-й аргумент — не массив
SELECT array_slice(5431, 1, 2);

-- start — не integer
SELECT array_slice(ARRAY[1,2,3], 'broken', 2);

-- end — не integer
SELECT array_slice(ARRAY[1,2,3], 1, 'broken');
