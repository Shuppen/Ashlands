--[[
  mods/core/biomes/biomes.lua
  Определения биомов.
--]]

local function biome(def)
    register_biome and register_biome(def)
end

biome({
    id   = "ashland",
    name = "Пепельные пустоши",
    description = "Выжженная, покрытая пеплом земля. Здесь ничего не растёт.",
    floor_tile  = TILE_ASH,
    wall_tile   = TILE_WALL,
    ambient_color = { 0.6, 0.55, 0.5 },
    fog_density   = 0.3,
    creatures = {
        { id = "ash_wolf", weight = 30 },
        { id = "ash_rat",  weight = 60 },
    },
    ground_textures = { "ash_stone", "ash_floor" },
})

biome({
    id   = "dead_forest",
    name = "Мёртвый лес",
    description = "Обугленные стволы деревьев. Птиц здесь нет.",
    floor_tile  = TILE_ASH,
    wall_tile   = TILE_TREE,
    ambient_color = { 0.4, 0.4, 0.35 },
    fog_density   = 0.5,
    creatures = {
        { id = "ash_wolf", weight = 40 },
        { id = "ash_rat",  weight = 30 },
    },
    ground_textures = { "dead_grass", "ash_floor" },
})

biome({
    id   = "ruins",
    name = "Руины",
    description = "Обломки некогда великого города. В них скрывается опасность.",
    floor_tile  = TILE_FLOOR,
    wall_tile   = TILE_WALL,
    ambient_color = { 0.5, 0.45, 0.40 },
    fog_density   = 0.2,
    creatures = {
        { id = "ash_rat",  weight = 70 },
        { id = "ash_wolf", weight = 20 },
    },
    ground_textures = { "ruin_wall", "ash_stone" },
})

biome({
    id   = "dungeon",
    name = "Подземелье",
    description = "Тёмные коридоры под пустошами. Только факел может спасти.",
    floor_tile  = TILE_FLOOR,
    wall_tile   = TILE_WALL,
    ambient_color = { 0.1, 0.1, 0.12 },
    fog_density   = 0.1,
    creatures = {
        { id = "ash_rat",  weight = 80 },
    },
    ground_textures = { "ash_stone" },
})
