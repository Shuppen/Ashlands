--[[
  ШАБЛОН ПРЕДМЕТА ДЛЯ ASHLANDS
  ================================
  Скопируй в mods/<твой_мод>/items/<имя>.lua и заполни поля.

  Скажи AI: «Создай предмет [описание] для Ashlands»
--]]

register_item({
    id     = "my_item",         -- уникальный ID
    name   = "Мой предмет",     -- отображаемое имя

    -- Тип: "food" | "weapon" | "armor" | "material" |
    --       "consumable" | "tool" | "quest"
    type   = "consumable",

    value  = 10,                -- стоимость в монетах
    weight = 1,                 -- вес (0 = невесомый)

    -- stack_size = 10,         -- сколько штук в стопке (по умолчанию 1)

    -- Как выглядит
    render = {
        ascii = { glyph = "!", color = 0xFFFF00 },
        -- tile_2d = { texture = "my_item_tex", sprite = 0 },
    },

    -- Описание для инвентаря
    description = "Загадочный предмет из пустошей.",

    -- Что происходит при использовании (опционально)
    use_effect = function(user_id)
        entity_heal(user_id, 15)
        ui_log("Вы используете " .. "мой предмет" .. ".", COL_WHITE)
    end,

    -- Бонусы при экипировке (для оружия/брони)
    -- equip_bonuses = {
    --     attack  = 3,
    --     defense = 0,
    --     speed   = 1,
    -- },
})
