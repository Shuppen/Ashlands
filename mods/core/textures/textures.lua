--[[
  mods/core/textures/textures.lua
  Core procedural texture definitions.
  Each texture is described by layers; the engine generates the
  actual pixel data at load time and caches it.
--]]

-- Пепельный камень — базовая поверхность пустошей
register_texture and register_texture("ash_stone", {
    size = 32,
    base_color = {0.35, 0.32, 0.28},
    layers = {
        {
            type      = "perlin",
            scale     = 4.0,
            intensity = 0.3,
            color_var = {0.05, 0.05, 0.03},
        },
        {
            type       = "voronoi",
            scale      = 6.0,
            edge_width = 0.05,
            edge_color = {0.15, 0.12, 0.10},
        },
        {
            type      = "perlin",
            scale     = 2.0,
            intensity = 0.15,
            color     = {0.55, 0.50, 0.45},
            blend     = "overlay",
        },
    },
    tiling   = true,
    roughness = 0.7,
})

-- Пепел — поверхность пола в пустошах
register_texture and register_texture("ash_floor", {
    size = 32,
    base_color = {0.50, 0.48, 0.44},
    layers = {
        {
            type      = "perlin",
            scale     = 3.0,
            intensity = 0.2,
            color_var = {0.04, 0.04, 0.03},
        },
        {
            type      = "perlin",
            scale     = 8.0,
            intensity = 0.1,
            color     = {0.40, 0.38, 0.35},
        },
    },
    tiling = true,
})

-- Трава мертвая — почерневшая, выжженная
register_texture and register_texture("dead_grass", {
    size = 32,
    base_color = {0.28, 0.26, 0.18},
    layers = {
        {
            type      = "perlin",
            scale     = 5.0,
            intensity = 0.25,
            color_var = {0.06, 0.05, 0.03},
        },
        {
            type      = "cellular",
            scale     = 10.0,
            intensity = 0.15,
            color     = {0.22, 0.20, 0.12},
        },
    },
    tiling = true,
})

-- Кирпичная стена — для руин
register_texture and register_texture("ruin_wall", {
    size = 32,
    base_color = {0.45, 0.38, 0.30},
    layers = {
        {
            type       = "voronoi",
            scale      = 4.0,
            edge_width = 0.08,
            edge_color = {0.20, 0.15, 0.10},
        },
        {
            type      = "perlin",
            scale     = 8.0,
            intensity = 0.1,
            color_var = {0.05, 0.04, 0.03},
        },
    },
    tiling = true,
    normal_map = true,
})
