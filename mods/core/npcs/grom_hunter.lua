if register_npc then register_npc({
    id = "grom_hunter",
    name = "Гром",
    faction = "hunters_guild",
    render = {
        ascii = { glyph = "G", color = 0x80A060 },
    },
    dialog = {
        start = {
            text = "Путник... Ты выглядишь голодным. Или смелым.",
            options = {
                {
                    text = "Есть работа?",
                    next = "quest_offer",
                    condition = function()
                        return not quest_active("first_hunt") and quest_available("first_hunt")
                    end,
                },
                {
                    text = "Я принёс шкуры.",
                    next = nil,
                    condition = function()
                        return quest_active("first_hunt")
                    end,
                    effect = function()
                        quest_advance("first_hunt")
                    end,
                },
                { text = "Что тут произошло?", next = "lore_ashlands" },
                { text = "[Уйти]", next = nil },
            },
        },
        quest_offer = {
            text = "Волки обнаглели. Принеси три шкуры — получишь награду.",
            options = {
                {
                    text = "Договорились.",
                    next = nil,
                    effect = function()
                        quest_start("first_hunt")
                    end,
                },
                { text = "Не сейчас.", next = nil },
            },
        },
        lore_ashlands = {
            text = "Пепел был не всегда. Старики говорят, раньше тут были леса.",
            options = {
                { text = "Разлом?", next = "lore_rift" },
                { text = "Понятно.", next = nil },
            },
        },
        lore_rift = {
            text = "Разлом пришёл вместе с огнём и железом. Всё после него стало чужим.",
            options = {
                { text = "Спасибо.", next = nil },
            },
        },
    },
}) end
