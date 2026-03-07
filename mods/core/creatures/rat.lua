--[[
  mods/core/creatures/rat.lua
  Пепельная крыса — первый враг в игре.
  Управляется полностью через Lua.
--]]

local Rat = {}

-- Текстура меха
register_texture and register_texture("rat_fur", {
    size = 16,
    base_color = {0.30, 0.28, 0.25},
    layers = {
        {
            type      = "perlin",
            scale     = 10.0,
            intensity = 0.15,
            color_var = {0.05, 0.04, 0.03},
        },
    },
})

-- Регистрация существа
register_creature and register_creature({
    id   = "ash_rat",
    name = "Пепельная крыса",

    render = {
        ascii    = { glyph = "r", color = 0x6A6050 },
        tile_2d  = { texture = "rat_fur" },
        iso_25d  = { texture = "rat_fur" },
        lowpoly_3d = { texture = "rat_fur" },
    },

    stats = { hp = 8, attack = 3, defense = 1, speed = 6 },
    tags  = { "animal", "vermin", "small" },

    loot = {
        { item = "rat_meat",  chance = 1.0 },
        { item = "rat_tail",  chance = 0.5 },
    },

    spawn = {
        biomes  = { "dungeon", "ruins", "sewer" },
        time    = "any",
        group   = { min = 1, max = 4 },
        rarity  = 0.7,
    },
})

-- AI поведение
function Rat.on_turn(self_id)
    local x, y = entity_get_pos(self_id)
    local nearby = entity_find_nearby(x, y, 6, "player")

    if #nearby > 0 then
        local px, py = entity_get_pos(nearby[1])
        -- Двигаться к игроку
        local dx = px - x
        local dy = py - y
        if math.abs(dx) > math.abs(dy) then
            entity_set_pos(self_id, x + (dx > 0 and 1 or -1), y)
        else
            entity_set_pos(self_id, x, y + (dy > 0 and 1 or -1))
        end
    else
        -- Случайное блуждание
        local dirs = { {1,0}, {-1,0}, {0,1}, {0,-1} }
        local d = dirs[math.random(#dirs)]
        local nx = x + d[1]
        local ny = y + d[2]
        if map_is_walkable(nx, ny) then
            entity_set_pos(self_id, nx, ny)
        end
    end
end

function Rat.on_death(self_id)
    local x, y = entity_get_pos(self_id)
    ui_log("Крыса издаёт последний писк.", 0x808080)
end

return Rat
