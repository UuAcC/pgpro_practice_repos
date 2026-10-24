-- -----------------------------------------------------------
--                     Обычные случаи:
-- -----------------------------------------------------------

-- 5.1 array_pad(anyarray, integer, anynonarray)

-- Дополнение в конец
SELECT array_pad(ARRAY[1,2], 4, 0);
-- Ожидается: {1,2,0,0}

-- Дополнение в начало (отрицательный size)
SELECT array_pad(ARRAY[1,2], -4, 0);
-- Ожидается: {0,0,1,2}

-- size == длине массива (ничего не меняется)
SELECT array_pad(ARRAY[1,2], 2, 0);
-- Ожидается: {1,2}

-- Пустой оригинальный массив
SELECT array_pad(ARRAY[]::int[], 3, 7);
-- Ожидается: {7,7,7}

-- Пустой оригинальный массив, отрицательный size
SELECT array_pad(ARRAY[]::int[], -3, 7);
-- Ожидается: {7,7,7}

-- size = 0 с пустым массивом
SELECT array_pad(ARRAY[]::int[], 0, 7);
-- Ожидается: {}

-- Текстовый массив
SELECT array_pad(ARRAY['a','b'], 4, 'x');
-- Ожидается: {a,b,x,x}

-- Исходный массив с NULL-элементами (NULL сохраняются)
SELECT array_pad(ARRAY[1,NULL,3], 5, 0);
-- Ожидается: {1,NULL,3,0,0}

-- Большой массив (производительность)
SELECT array_pad(ARRAY[1,2,3], 10000, 0);

-- 5.2: N-мерный исходный массив с согласованной формой
SELECT array_pad(ARRAY[[1,2],[3,4]], 3, ARRAY[0,0]);
-- Ожидается: {{1,2},{3,4},{0,0}}

-- 5.2: N-мерный исходный, дополнение в начало
SELECT array_pad(ARRAY[[1,2],[3,4]], -3, ARRAY[0,0]);
-- Ожидается: {{0,0},{1,2},{3,4}}

-- 5.2: Пустой исходный массив — результат строится от добавляемого
SELECT array_pad(ARRAY[]::int[], 3, ARRAY[6,7]);
-- Ожидается: {{6,7},{6,7},{6,7}}

-- 5.2: Исходный массив с NULL-элементами (NULL сохраняются)
SELECT array_pad(ARRAY[[1,NULL],[NULL,2]], 4, ARRAY[0,0]);
-- Ожидается: {{1,NULL},{NULL,2},{0,0},{0,0}}

-- 5.2: Добавляемый массив с NULL-элементами (NULL сохраняются)
SELECT array_pad(ARRAY[[1,2],[3,2]], 4, ARRAY[NULL,5]);
-- Ожидается: {{1,2},{3,4},{NULL,6},{NULL,6}}

-- 5.2: Большой массив (производительность)
SELECT array_pad(ARRAY[[1,2],[3,4]], 5000, ARRAY[0,0]);

-- -----------------------------------------------------------
--                    Обработка ошибок:
-- -----------------------------------------------------------

-- 5.1: size = 0 с непустым массивом
SELECT array_pad(ARRAY[1,2], 0, 7);
-- Ошибка: The new lenght of the array can't be less than current

-- 5.1: |size| < длины массива
SELECT array_pad(ARRAY[1,2,3], 2, 0);
-- Ошибка: The new lenght of the array can't be less than current

-- 5.1: тип value не соответствует типу массива
SELECT array_pad(ARRAY[1,2], 4, 'a');
-- Ошибка: The element to add should have the same type as array elements

-- 5.1: N-мерный массив — ошибка
SELECT array_pad(ARRAY[[1,2],[3,4]], 3, 0);
-- Ошибка: The argument array should be one-dimensioned

-- 5.2: size = 0 с непустым исходным
SELECT array_pad(ARRAY[[1,2]], 0, ARRAY[0,0]);
-- Ошибка: The new lenght of the array can't be less than current

-- 5.2: |size| < длины по первому измерению
SELECT array_pad(ARRAY[[1,2],[3,4]], 1, ARRAY[0,0]);
-- Ошибка: The new lenght of the array can't be less than current

-- 5.2: добавляемый массив пустой
SELECT array_pad(ARRAY[1,2], 4, ARRAY[]::int[]);
-- Ошибка: The array being added to the original array shouldn't be empty

-- 5.2: несогласованная размерность
SELECT array_pad(ARRAY[[1,2],[3,4]], 3, ARRAY[[0,0],[0,0]]);
-- Ошибка: The array being added to the original array should have one dimension fewer than the original

-- 5.2: несогласованные размеры по оставшимся измерениям
SELECT array_pad(ARRAY[[1,2],[3,4]], 3, ARRAY[0,0,0]);
-- Ошибка: ...should have a numbers of elements equal to the numbers of elements in the original array in all dimensions except the first

-- 5.2: несогласованные lower bounds
SELECT array_pad('[0:1][1:2]={{1,2},{3,4}}'::int[], 3, '[2:3]={0,0}'::int[]);
-- Ошибка: ...should have a lower bounds equal to the lower bounds in the original array in all dimensions except the first

-- 5.2: тип элементов добавляемого не совпадает
SELECT array_pad(ARRAY[1,2], 4, ARRAY['a']);
-- Ошибка: The array being added to the original array should have the same type as the original

-- 1-й аргумент — не массив
SELECT array_pad(5431, 4, 0);

-- size — не integer
SELECT array_pad(ARRAY[1,2], 'broken', 0);

