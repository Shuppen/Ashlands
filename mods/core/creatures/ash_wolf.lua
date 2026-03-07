--[[
  mods/core/creatures/ash_wolf.lua
  Пепельный волк — ночной хищник, охотится стаями.
  Боится огня. Пример из мастер-промпта.
--]]

local AshWolf = {}

-- Процедурная текстура меха
register_texture and register_texture("ash_wolf_fur", {
    size = 32,
    base_color = {0.4, 0.38, 0.35},
    layers = {
        {
            type      = "perlin",
            scale     = 8,
            intensity = 0.2,
            color_var = {0.10, 0.08, 0.05},
        },
        {
            type      = "cellular",
            scale     = 12,
            intensity = 0.10,
            color     = {0.30, 0.28, 0.25},
        },
    },
})

-- Регистрация существа
register_creature and register_creature({
    id   = "ash_wolf",
    name = "Пепельный волк",

    render = {
        ascii      = { glyph = "w", color = 0xA09080 },
        tile_2d    = { texture = "ash_wolf_fur" },
        iso_25d    = { texture = "ash_wolf_fur" },
        lowpoly_3d = { texture = "ash_wolf_fur" },
    },

    stats = { hp = 35, attack = 8, defense = 3, speed = 7 },
    tags  = { "animal", "pack", "nocturnal", "predator" },

    loot = {
        { item = "wolf_pelt", chance = 0.8 },
        { item = "ash_fang",  chance = 0.3 },
        { item = "raw_meat",  chance = 1.0 },
    },

    spawn = {
        biomes  = { "ashland", "dead_forest", "ruins" },
        time    = "night",
        group   = { min = 2, max = 5 },
        rarity  = 0.4,
    },
})

-- AI: боится огня, охотится стаями
function AshWolf.on_see(self_id, target_id)
    if entity_has_tag(target_id, "fire") then
        local tx, ty = entity_get_pos(target_id)
        local sx, sy = entity_get_pos(self_id)
        local dx = sx - tx
        local dy = sy - ty
        entity_set_pos(self_id, sx + (dx > 0 and 1 or -1),
                                sy + (dy > 0 and 1 or -1))
        return
    end

    if entity_has_tag(target_id, "player") and world_is_night() then
        -- Привлекает стаю
        local x, y = entity_get_pos(self_id)
        local pack = entity_find_nearby(x, y, 15, "ash_wolf")
        ui_log("Вы слышите волчий вой...", 0x808080)
    end
end

function AshWolf.on_turn(self_id)
    local x, y  = entity_get_pos(self_id)
    local pack  = entity_find_nearby(x, y, 8, "ash_wolf")
    local nearby = entity_find_nearby(x, y, 8, "player")

    if #pack >= 3 and #nearby > 0 and world_is_night() then
        -- Атаковать
        local px, py = entity_get_pos(nearby[1])
        local dx = px - x
        local dy = py - y
        if math.abs(dx) + math.abs(dy) <= 1 then
            entity_damage(nearby[1], 8)
        else
            if math.abs(dx) > math.abs(dy) then
                entity_set_pos(self_id, x + (dx > 0 and 1 or -1), y)
            else
                entity_set_pos(self_id, x, y + (dy > 0 and 1 or -1))
            end
        end
    else
        -- Блуждать
        local dirs = { {1,0}, {-1,0}, {0,1}, {0,-1} }
        local d = dirs[math.random(#dirs)]
        local nx = x + d[1]
        local ny = y + d[2]
        if map_is_walkable(nx, ny) then
            entity_set_pos(self_id, nx, ny)
        end
    end
end

function AshWolf.on_death(self_id)
    ui_log("Пепельный волк испускает последний вой...", 0x808080)
    -- Оставшаяся стая становится агрессивнее
    local x, y = entity_get_pos(self_id)
    local pack = entity_find_nearby(x, y, 12, "ash_wolf")
    for _, wolf_id in ipairs(pack) do
        -- Увеличить агрессию (упрощённо через тег)
        entity_add_tag(wolf_id, "enraged")
    end
end

return AshWolf
