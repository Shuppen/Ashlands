# Как создать мод для Ashlands

Ashlands создан для **вайб-кодеров** — людей, которые работают с AI-ассистентами.
Ты **не обязан** знать C или разбираться в движке. Весь контент создаётся на Lua.

---

## Быстрый старт

1. Создай папку `mods/<твой_мод>/`
2. Создай файл `mods/<твой_мод>/mod.lua`
3. Добавь свой мод в `mods/core/init.lua`

```lua
-- В mods/core/init.lua добавь строку:
dofile("mods/my_mod/mod.lua")
```

---

## Что можно создавать

| Тип | Шаблон | Команда AI |
|-----|--------|-----------|
| Существо | `creature_template.lua` | «Создай существо [описание]» |
| Предмет | `item_template.lua` | «Создай предмет [описание]» |
| Биом | `biome_template.lua` | «Создай биом [название]» |
| Квест | `quest_template.lua` | «Создай квест [описание]» |
| Текстура | `texture_template.lua` | «Создай текстуру [материал]» |

---

## Пример: создать крысу за 5 минут

Скопируй `creature_template.lua` в `mods/my_mod/rat.lua`.

Скажи AI:
> «Я делаю мод для Ashlands. Создай существо "Пепельная крыса-мутант":
> большая, агрессивная, выпадает мутировавшее мясо. Используй шаблон
> creature_template.lua»

AI создаст готовый файл — просто сохрани его.

---

## Доступные функции Lua API

### Сущности
```lua
entity_spawn(type_id, x, y)         -- создать сущность
entity_destroy(entity_id)           -- уничтожить
entity_get_pos(entity_id)           -- → x, y
entity_set_pos(entity_id, x, y)     -- переместить
entity_get_hp(entity_id)            -- → current, max
entity_damage(entity_id, amount)    -- нанести урон
entity_heal(entity_id, amount)      -- вылечить
entity_has_tag(entity_id, tag)      -- → bool
entity_add_tag(entity_id, tag)      -- добавить тег
entity_find_nearby(x, y, r, tag)    -- → {ids}
```

### Карта
```lua
map_get_tile(x, y)                  -- → tile_type
map_set_tile(x, y, tile_type)       -- изменить тайл
map_is_walkable(x, y)               -- → bool
```

### Мир и время
```lua
world_get_time()                    -- → hour, day, season
world_is_night()                    -- → bool
world_get_weather()                 -- → weather_type
```

### UI
```lua
ui_log(text, color)                 -- сообщение в лог
ui_show_message(text)               -- уведомление
```

### Квесты, NPC и фракции
```lua
quest_start(quest_id)               -- запустить квест
quest_active(quest_id)              -- → bool
quest_available(quest_id)           -- → bool
quest_advance(quest_id)             -- перейти к следующему этапу
quest_fail(quest_id)                -- провалить квест

npc_start_dialog(npc_id)            -- открыть диалог
npc_dialog_open()                   -- → bool

faction_get_rep(faction_id)         -- → репутация игрока
faction_change_rep(faction_id, d)   -- изменить репутацию

player_level()                      -- → уровень игрока
```

### Константы тайлов
```lua
TILE_FLOOR, TILE_WALL, TILE_WATER, TILE_ASH,
TILE_STAIRS_UP, TILE_STAIRS_DOWN,
TILE_DOOR_OPEN, TILE_DOOR_SHUT
```

### Константы цветов
```lua
COL_WHITE, COL_RED, COL_GREEN, COL_BLUE,
COL_YELLOW, COL_GRAY, COL_BROWN
```

---

## Правила хорошего мода

- Один файл = один тип контента (крыса — rat.lua, не all.lua)
- Используй уникальные `id` (добавь префикс: `mymod_rat`)
- Тестируй в ASCII-режиме (быстрее всего)
- Если что-то сломалось — смотри консоль, там будет ошибка Lua

## Как просить AI точнее

- `Сделай из шаблона предмет для ранней игры, без новых API`
- `Сделай из шаблона квест в 2 этапа с talk_to и collect`
- `Сделай процедурную текстуру пепельного камня, тайлимую, 32x32`
- `Сделай NPC с 3 вариантами реплик и условием quest_active`

---

## Помощь и вопросы

- Issues: https://github.com/[repo]/Ashlands/issues
- Говори AI что именно хочешь и в каком файле шаблона это сделать
- Вся игровая логика в mods/ — движок трогать не нужно
