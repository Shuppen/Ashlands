if register_faction then register_faction({
    id = "hunters_guild",
    name = "Гильдия охотников",
    relations = {
        ash_nomads = 0.3,
        iron_cult = -0.7,
    },
    territory_biomes = { "ashland", "dead_forest", "ruins" },
    default_player_rep = 0,
}) end

if register_faction then register_faction({
    id = "ash_nomads",
    name = "Пепельные кочевники",
    relations = {
        hunters_guild = 0.4,
        iron_cult = -0.8,
    },
    territory_biomes = { "ashland", "dead_forest" },
    default_player_rep = 0,
}) end

if register_faction then register_faction({
    id = "iron_cult",
    name = "Железный культ",
    relations = {
        hunters_guild = -0.8,
        ash_nomads = -0.7,
    },
    territory_biomes = { "ruins", "dungeon" },
    default_player_rep = -10,
}) end
