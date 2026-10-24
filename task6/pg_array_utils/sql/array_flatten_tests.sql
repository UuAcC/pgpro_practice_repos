-- -----------------------------------------------------------
--                     Обычные случаи:
-- -----------------------------------------------------------

-- 2D массив integer с NULL элементами
SELECT array_flatten(ARRAY[[1,2,NULL],[4,NULL,6]]);
-- Ожидается: {1,2,NULL,4,NULL,6}

-- 3D массив text 
SELECT array_flatten(ARRAY[[['Я','помню'],['чудное','мгновенье:']],[['Передо','мной'],['явилась','ты...']]]);
-- Ожидается: {'Я','помню','чудное','мгновенье:','Передо','мной','явилась','ты...'}

-- 1D массив integer 
SELECT array_flatten(ARRAY[1,1,2,3,5,8,13,21]);
-- Ожидается: {1,1,2,3,5,8,13,21} (возврат ввода)

-- Пустой массив integer 
SELECT array_flatten(ARRAY[]::int[]);
-- Ожидается: {} (возврат ввода)

-- Большой 2D массив integer (производительность)
SELECT array_flatten(ARRAY[
    ARRAY(SELECT generate_series(1, 2000)),
    ARRAY(SELECT generate_series(1, 2000)),
    ARRAY(SELECT generate_series(1, 2000)),
    ARRAY(SELECT generate_series(1, 2000)),
    ARRAY(SELECT generate_series(1, 2000))
]);

-- -----------------------------------------------------------
--                    Обработка ошибок:
-- -----------------------------------------------------------

-- Аргумент - не массив
SELECT array_flatten(5431);
-- Ошибка: function array_flatten(integer) does not exist

