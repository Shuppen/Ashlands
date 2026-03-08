--[[
  mods/core/init.lua — Core content loader
  Loaded automatically by the engine at startup.
  All canonical game content is registered here.
--]]

-- Load sub-modules in order
local modules = {
    "mods/core/textures/textures.lua",
    "mods/core/creatures/rat.lua",
    "mods/core/creatures/ash_wolf.lua",
    "mods/core/items/items.lua",
    "mods/core/biomes/biomes.lua",
    "mods/core/factions/factions.lua",
    "mods/core/quests/first_hunt.lua",
    "mods/core/npcs/grom_hunter.lua",
}

for _, path in ipairs(modules) do
    local ok, err = pcall(dofile, path)
    if not ok then
        ui_log("[core] Failed to load " .. path .. ": " .. tostring(err), COL_RED)
    end
end

ui_log("[core] Core mods loaded.", COL_GRAY)
