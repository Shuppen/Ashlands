--[[
  ШАБЛОН СУЩЕСТВА ДЛЯ ASHLANDS
  ================================
  Скопируй этот файл в mods/<твой_мод>/creatures/<имя>.lua
  и заполни поля.

  Скажи AI: «Создай существо [описание] для Ashlands»
  и получишь готовый файл на основе этого шаблона.
--]]

-- Опционально: процедурная текстура
register_texture("my_creature_texture", {
    size       = 32,
    base_color = { 0.5, 0.5, 0.5 },  -- RGB [0..1]
    layers = {
        {
            type      = "perlin",    -- "perlin" | "voronoi" | "cellular"
            scale     = 5.0,
            intensity = 0.2,
            color_var = { 0.05, 0.05, 0.05 },
        },
    },
})

-- Регистрация существа
register_creature({
    id   = "my_creature",          -- уникальный ID
    name = "Моё существо",         -- отображаемое имя

    -- Как выглядит в каждом режиме рендера
    render = {
        ascii      = { glyph = "m", color = 0xAAAAAA },
        tile_2d    = { texture = "my_creature_texture" },
        iso_25d    = { texture = "my_creature_texture" },
        lowpoly_3d = { texture = "my_creature_texture" },
    },

    -- Базовые характеристики
    stats = {
        hp      = 20,    -- здоровье
        attack  = 5,     -- атака
        defense = 2,     -- защита
        speed   = 4,     -- скорость (1..10)
    },

    -- Теги — влияют на AI и взаимодействия
    tags = { "animal" },
    -- Доступные теги: animal, undead, fire, nocturnal, pack,
    --                 predator, scavenger, small, large, flying

    -- Что выпадает при гибели
    loot = {
        { item = "raw_meat", chance = 1.0 },  -- 100%
        -- { item = "bone",  chance = 0.5 },   -- 50%
    },

    -- Условия спавна
    spawn = {
        biomes = { "ashland" },  -- в каких биомах
        time   = "any",          -- "any" | "day" | "night"
        group  = { min = 1, max = 1 },  -- размер группы
        rarity = 0.5,            -- 0..1  (1 = очень часто)
    },
})

-- =========================================================
-- AI-поведение (опционально)
-- Все функции получают entity_id первым аргументом.
-- =========================================================

-- Вызывается каждый ход
function on_turn(self_id)
    local x, y = entity_get_pos(self_id)

    -- Искать ближайшего игрока
    local targets = entity_find_nearby(x, y, 5, "player")
    if #targets > 0 then
        local px, py = entity_get_pos(targets[1])
        local dx = px - x
        local dy = py - y
        -- Двигаться в сторону цели
        if math.abs(dx) > math.abs(dy) then
            entity_set_pos(self_id, x + (dx > 0 and 1 or -1), y)
        else
            entity_set_pos(self_id, x, y + (dy > 0 and 1 or -1))
        end
    else
        -- Случайное блуждание
        local dirs = { {1,0},{-1,0},{0,1},{0,-1} }
        local d = dirs[math.random(#dirs)]
        if map_is_walkable(x + d[1], y + d[2]) then
            entity_set_pos(self_id, x + d[1], y + d[2])
        end
    end
end

-- Вызывается когда существо умирает
function on_death(self_id)
    ui_log("Существо погибает!", COL_GRAY)
end

-- Вызывается когда существо замечает цель
function on_see(self_id, target_id)
    -- Можно добавить особое поведение
end

-- Скажи AI: «Сделай из этого шаблона ночного падальщика,
-- который спавнится стаями и выпадает 2 материала»
