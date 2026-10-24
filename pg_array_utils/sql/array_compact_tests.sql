-- -----------------------------------------------------------
--                     Обычные случаи:
-- -----------------------------------------------------------

-- Режим 'null' (по умолчанию): удаление NULL-элементов
SELECT array_compact(ARRAY[1,NULL,3]);
-- Ожидается: {1,3}

-- Режим 'null' для текстового массива
SELECT array_compact(ARRAY['a',NULL,'b'], 'null');
-- Ожидается: {a,b}

-- Режим 'zero' для массива integer
SELECT array_compact(ARRAY[0,1,2,0,3], 'zero');
-- Ожидается: {1,2,3}

-- Режим 'zero' для массива numeric
SELECT array_compact(ARRAY[0.0, 1.5, 0, 2.5]::numeric[], 'zero');
-- Ожидается: {1.5,2.5}

-- Режим 'zero': NULL-элементы сохраняются
SELECT array_compact(ARRAY[0,NULL,2], 'zero');
-- Ожидается: {NULL,2}

-- Режим 'empty' для массива text
SELECT array_compact(ARRAY['a','','b',''], 'empty');
-- Ожидается: {a,b}

-- Режим 'empty' для массива varchar
SELECT array_compact(ARRAY['x','','y']::varchar[], 'empty');
-- Ожидается: {x,y}

-- Режим 'empty' для массива bytea
SELECT array_compact(ARRAY['\x'::bytea, '\xdeadbeef'::bytea, '\x'::bytea], 'empty');
-- Ожидается: {"\\xdeadbeef"}

-- Режим 'empty': NULL-элементы сохраняются
SELECT array_compact(ARRAY['a',NULL,''], 'empty');
-- Ожидается: {a,NULL}

-- Режим 'falsy' для массива boolean
SELECT array_compact(ARRAY[true,false,true], 'falsy');
-- Ожидается: {t,t}

-- Режим 'falsy' для массива integer (удаляет NULL и 0)
SELECT array_compact(ARRAY[0,1,NULL,2], 'falsy');
-- Ожидается: {1,2}

-- Режим 'falsy' для массива text (удаляет NULL и '')
SELECT array_compact(ARRAY['a','',NULL,'b'], 'falsy');
-- Ожидается: {a,b}

-- Режим 'falsy' для типа, не являющегося bool/numeric/text
-- (фильтрует только NULL, аналогично 'null')
SELECT array_compact(
    ARRAY['2024-01-01'::date, '2024-01-02'::date, NULL],
    'falsy'
);
-- Ожидается: {01-01-2024,01-02-2024}

-- Все элементы отфильтрованы — пустой массив
SELECT array_compact(ARRAY[0,0,0], 'zero');
-- Ожидается: {}

SELECT array_compact(ARRAY['',''], 'empty');
-- Ожидается: {}

SELECT array_compact(ARRAY[NULL,NULL]::int[], 'null');
-- Ожидается: {}

-- Пустой массив — возврат ввода после проверки режима/типа
SELECT array_compact(ARRAY[]::int[], 'zero');
-- Ожидается: {}

-- Большой массив integer с большим числом нулей (производительность)
SELECT array_compact(
    ARRAY(SELECT CASE WHEN i % 3 = 0 THEN 0 ELSE i END FROM generate_series(1,10000) AS i),
    'zero'
);

-- -----------------------------------------------------------
--                    Обработка ошибок:
-- -----------------------------------------------------------

-- Непустой N-мерный массив
SELECT array_compact(ARRAY[[1,2],[3,4]], 'null');
-- Ошибка: The argument array must be one-dimensional

-- Несоответствие режима типу: 'zero' для text
SELECT array_compact(ARRAY['a','b'], 'zero');
-- Ошибка: Impossible to use zero mode with text array

-- Несоответствие режима типу: 'empty' для integer
SELECT array_compact(ARRAY[1,2], 'empty');
-- Ошибка: Impossible to use empty mode with integer array

-- Несуществующий режим
SELECT array_compact(ARRAY[1,2], 'invalid');
-- Ошибка: invalid is not an existing mode

-- Несуществующий режим (чувствительность к регистру)
SELECT array_compact(ARRAY[1,2], 'NULL');
-- Ошибка: NULL is not an existing mode

-- 1-й аргумент — не массив
SELECT array_compact(5431, 'null');

-- mode — не строка
SELECT array_compact(ARRAY[1,2], 42);

