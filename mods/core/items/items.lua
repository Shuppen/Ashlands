--[[
  mods/core/items/items.lua
  Базовые предметы игры.
--]]

local function item(def)
    register_item and register_item(def)
end

-- === Еда ===
item({
    id     = "raw_meat",
    name   = "Сырое мясо",
    type   = "food",
    value  = 5,
    weight = 2,
    render = { ascii = { glyph = "%", color = 0xCC6666 } },
    use_effect = function(user_id)
        entity_heal(user_id, 8)
        ui_log("Вы съели сырое мясо. Противно, но питательно.", 0xCC6666)
    end,
})

item({
    id     = "rat_meat",
    name   = "Мясо крысы",
    type   = "food",
    value  = 2,
    weight = 1,
    render = { ascii = { glyph = "%", color = 0xAA5555 } },
    use_effect = function(user_id)
        entity_heal(user_id, 4)
        ui_log("Крысятина. Не деликатес, но голод не тётка.", 0xAA5555)
    end,
})

-- === Материалы ===
item({
    id     = "rat_tail",
    name   = "Хвост крысы",
    type   = "material",
    value  = 1,
    weight = 0,
    render = { ascii = { glyph = "~", color = 0x888888 } },
})

item({
    id     = "wolf_pelt",
    name   = "Шкура волка",
    type   = "material",
    value  = 15,
    weight = 5,
    render = { ascii = { glyph = "[", color = 0x907060 } },
})

item({
    id     = "ash_fang",
    name   = "Пепельный клык",
    type   = "material",
    value  = 8,
    weight = 1,
    render = { ascii = { glyph = "!", color = 0xCCBBAA } },
})

-- === Оружие ===
item({
    id     = "wooden_club",
    name   = "Деревянная дубина",
    type   = "weapon",
    value  = 10,
    weight = 4,
    attack_bonus = 3,
    render = { ascii = { glyph = "/", color = 0x886633 } },
})

item({
    id     = "bone_knife",
    name   = "Костяной нож",
    type   = "weapon",
    value  = 18,
    weight = 1,
    attack_bonus = 5,
    render = { ascii = { glyph = "/", color = 0xDDCCBB } },
})

-- === Расходники ===
item({
    id     = "bandage",
    name   = "Тряпичная повязка",
    type   = "consumable",
    value  = 12,
    weight = 0,
    render = { ascii = { glyph = "+", color = 0xFFFFFF } },
    use_effect = function(user_id)
        entity_heal(user_id, 20)
        ui_log("Перевязка сделана. Лучше так, чем ничего.", 0xFFFFFF)
    end,
})
