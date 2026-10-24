-- -----------------------------------------------------------
--                     Обычные случаи:
-- -----------------------------------------------------------

-- Два 1D массива разных типов
SELECT unnest(array_zip(ARRAY[1,2,3], ARRAY['a','b','c']));
-- Ожидается: {"f1":1,"f2":"a"}, {"f1":2,"f2":"b"}, {"f1":3,"f2":"c"}

-- Три 1D массива разных типов (int, text, bool)
SELECT unnest(array_zip(ARRAY[1,2], ARRAY['x','y'], ARRAY[true,false]));
-- Ожидается: {"f1":1,"f2":"x","f3":true}, {"f1":2,"f2":"y","f3":false}

-- Один из массивов пустой
SELECT unnest(array_zip(ARRAY[1,2,3], ARRAY[]::text[]));
-- Ожидается: {"f1":1,"f2":null}, {"f1":2,"f2":null}, {"f1":3,"f2":null}

-- Все массивы пустые
SELECT unnest(array_zip(ARRAY[]::int[], ARRAY[]::text[]));
-- Ожидается: пустой jsonb[]

-- Массивы с NULL-элементами
SELECT unnest(array_zip(ARRAY[1,NULL,3], ARRAY['a','b',NULL]));
-- Ожидается: {"f1":1,"f2":"a"}, {"f1":null,"f2":"b"}, {"f1":3,"f2":null}

-- N-мерный массив
SELECT unnest(array_zip(ARRAY[[1,2],[3,4]], ARRAY['a','b']));
-- Ожидается: {"f1":{1,2},"f2":"a"}, {"f1":{3,4},"f2":"b"}

-- Много больших 1D массивов integer (производительность)
SELECT unnest(array_zip(
    ARRAY(SELECT generate_series(1,1500)),
    ARRAY(SELECT generate_series(1,1500)),
    ARRAY(SELECT generate_series(1,1500)),
    ARRAY(SELECT generate_series(1,1500)),
    ARRAY(SELECT generate_series(1,1500)),
    ARRAY(SELECT generate_series(1,1500)),
    ARRAY(SELECT generate_series(1,1500)),
    ARRAY(SELECT generate_series(1,1500)),
    ARRAY(SELECT generate_series(1,1500)),
    ARRAY(SELECT generate_series(1,1500)),
));

-- -----------------------------------------------------------
--                    Обработка ошибок:
-- -----------------------------------------------------------

-- Нулевое число аргументов — ошибка (вызов без аргументов)
SELECT array_zip();
-- Ошибка: array_zip requires at least one array

