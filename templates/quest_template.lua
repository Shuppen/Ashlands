--[[
  ШАБЛОН КВЕСТА ДЛЯ ASHLANDS
  ================================
  Квесты — задачи с условиями завершения и наградами.

  Скажи AI: «Создай квест [название/описание] для Ashlands»
--]]

register_quest({
    id    = "my_quest",
    name  = "Название квеста",
    description = "Описание задачи.",
    giver = "npc_elder",       -- NPC, выдающий квест (опционально)

    -- Условия запуска
    trigger = {
        type = "talk",         -- "talk" | "enter_biome" | "kill" | "find"
        -- biome = "ashland",
        -- target = "npc_elder",
    },

    -- Этапы квеста
    stages = {
        {
            id          = "stage_1",
            description = "Убей 5 пепельных крыс.",
            condition   = {
                type   = "kill",
                target = "ash_rat",
                count  = 5,
            },
        },
        {
            id          = "stage_2",
            description = "Вернись к Старейшине.",
            condition   = {
                type   = "talk",
                target = "npc_elder",
            },
        },
    },

    -- Награда
    reward = {
        xp    = 50,
        items = {
            { id = "bandage", count = 3 },
        },
    },

    -- Хуки (опционально)
    on_start = function(player_id)
        ui_log("Квест начат: Название квеста.", COL_YELLOW)
    end,

    on_complete = function(player_id)
        ui_log("Квест выполнен! Получена награда.", COL_GREEN)
        entity_heal(player_id, 10)
    end,

    on_fail = function(player_id)
        ui_log("Квест провален.", COL_RED)
    end,
})
