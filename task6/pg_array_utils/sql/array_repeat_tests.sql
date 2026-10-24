-- -----------------------------------------------------------
--                     Обычные случаи:
-- -----------------------------------------------------------

-- 7.1: Повторение integer-значения
SELECT array_repeat(5, 3);
-- Ожидается: {5,5,5}

-- 7.1: Повторение text-значения
SELECT array_repeat('Around the world'::text, 2);
-- Ожидается: {"Around the world","Around the world"}

-- 7.1: Повторение boolean-значения
SELECT array_repeat(true, 3);
-- Ожидается: {t,t,t}

-- 7.1: count = 1
SELECT array_repeat(5, 1);
-- Ожидается: {5}

-- 7.1: count = 0
SELECT array_repeat(5, 0);
-- Ожидается: {}

-- 7.1: Большое значение count (производительность)
SELECT array_repeat('Around the world'::text, 10000);

-- 7.2: 1D массив
SELECT array_repeat(ARRAY[1,2], 3);
-- Ожидается: {{1,2},{1,2},{1,2}}

-- 7.2: 1D массив text
SELECT array_repeat(ARRAY['Around','the world'], 2);
-- Ожидается: {{Around,"the world"},{Around,"the world"}}

-- 7.2: N-мерный массив -> (N+1)-мерный
SELECT array_repeat(ARRAY[[1,2],[3,4]], 2);
-- Ожидается: {{{1,2},{3,4}},{{1,2},{3,4}}}

-- 7.2: count = 1
SELECT array_repeat(ARRAY[1,2], 1);
-- Ожидается: {{1,2}}

-- 7.2: count = 0
SELECT array_repeat(ARRAY[1,2], 0);
-- Ожидается: {}

-- 7.2: Пустой входной массив
SELECT array_repeat(ARRAY[]::int[], 5);
-- Ожидается: {}

-- 7.2: Массив с NULL-элементами (NULL сохраняются)
SELECT array_repeat(ARRAY[1,NULL], 2);
-- Ожидается: {{1,NULL},{1,NULL}}

-- 7.2: Большой массив (производительность)
SELECT array_repeat(ARRAY(SELECT generate_series(1,100)), 100);

-- -----------------------------------------------------------
--                    Обработка ошибок:
-- -----------------------------------------------------------

-- 7.1: count < 0
SELECT array_repeat(5, -1);
-- Ошибка: Count can't be negative

-- 7.2: count < 0
SELECT array_repeat(ARRAY[1,2], -1);
-- Ошибка: Count can't be negative

-- count — не integer
SELECT array_repeat(5, 'broken');
