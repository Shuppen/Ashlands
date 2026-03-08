if register_quest then register_quest({
    id = "first_hunt",
    name = "Первая охота",
    description = "Принеси Грому три волчьих шкуры.",
    available_when = function()
        return player_level() >= 1
    end,
    fail_conditions = {
        { type = "kill_npc", npc = "grom_hunter" },
    },
    stages = {
        {
            id = "start",
            text = "Гром: Принеси мне 3 волчьих шкуры.",
            objectives = {
                { type = "collect", item = "wolf_pelt", count = 3 },
            },
            on_complete = "return_to_grom",
        },
        {
            id = "return_to_grom",
            text = "Вернись к Грому со шкурами.",
            objectives = {
                { type = "talk_to", npc = "grom_hunter", count = 1 },
            },
            on_complete = "done",
        },
        {
            id = "done",
            type = "end",
            text = "Гром кивает и вручает тебе грубый топор.",
            rewards = {
                { type = "xp", amount = 50 },
                { type = "reputation", faction = "hunters_guild", amount = 10 },
            },
        },
    },
}) end
