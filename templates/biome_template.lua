--[[
  ШАБЛОН БИОМА ДЛЯ ASHLANDS
  ================================
  Биомы определяют внешний вид и содержимое регионов мира.

  Скажи AI: «Создай биом [название] для Ashlands»
--]]

register_biome({
    id   = "my_biome",          -- уникальный ID
    name = "Мой биом",          -- отображаемое имя
    description = "Описание биома.",

    -- Тип тайлов по умолчанию
    floor_tile = TILE_ASH,      -- TILE_FLOOR | TILE_ASH | TILE_GRASS | TILE_SAND
    wall_tile  = TILE_WALL,

    -- Цвет атмосферы (влияет на освещение)
    ambient_color = { 0.6, 0.55, 0.5 },

    -- Плотность тумана (0 = нет, 1 = очень густой)
    fog_density = 0.3,

    -- Существа в биоме (weight — относительный вес спавна)
    creatures = {
        { id = "ash_rat",  weight = 60 },
        { id = "ash_wolf", weight = 30 },
    },

    -- Текстуры поверхности (для процедурного рендера)
    ground_textures = { "ash_floor", "ash_stone" },

    -- Особые объекты (деревья, руины, точки интереса)
    -- objects = {
    --     { id = "dead_tree", density = 0.05 },
    --     { id = "ruin_pillar", density = 0.01 },
    -- },

    -- Ресурсы для добычи
    -- resources = {
    --     { id = "ash_stone", density = 0.2 },
    -- },
})
