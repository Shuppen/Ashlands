# ASHLANDS ENGINE — МАСТЕР-ПРОМПТ ДЛЯ РАЗРАБОТКИ ЯДРА

> Копируй этот промпт в начало каждой сессии с AI.
> Обновляй секцию [ТЕКУЩИЙ СТАТУС] по мере прогресса.

---

## КОНТЕКСТ ПРОЕКТА

Я разрабатываю **Ashlands** — open source roguelike-sandbox с элементами
выживания. Девиз: «В пепельных пустошах нет героев. Есть только те,
кто ещё не сдался.»

Проект строится вокруг идеи: сотни «вайб-кодеров» (люди без глубоких
навыков программирования, работающие через AI-ассистентов) создают
контент (мобы, предметы, квесты, биомы, текстуры) на Lua, а ядро
движка на C обеспечивает производительность и кроссплатформенность.

**Жанр:** Roguelike + Sandbox + Survival
**Аналоги:** Dwarf Fortress (глубина) × Terraria (песочница) ×
Caves of Qud (процедурный лор) × Cataclysm:DDA (модульность)
**Лицензия:** MIT

---

## ЦЕЛЕВОЕ ЖЕЛЕЗО (МИНИМУМ)

| Параметр | Значение |
|----------|----------|
| CPU | Intel Celeron B815 (Sandy Bridge), 2 ядра, 1.6 ГГц |
| RAM | 2 ГБ (реально доступно ~1.5 ГБ) |
| GPU | Intel HD Graphics (Sandy Bridge) |
| OpenGL | 3.1 (Linux Mesa), ES 3.0 |
| Экран | 1366×768 |
| Диск | HDD, медленный |
| ОС разработки | Linux Mint |

**Правило:** если работает на этом железе — работает везде.
Бюджет памяти движка: не более 200 МБ RAM.
Целевой FPS: 60 в 2D режимах, 30+ в 3D режиме.

---

## ТЕХНОЛОГИЧЕСКИЙ СТЕК

| Слой | Технология | Зачем |
|------|-----------|-------|
| Язык ядра | **C11** (не C++) | Максимальный контроль, быстрая компиляция |
| Окна/ввод/звук | **SDL2** | Кроссплатформа, лёгкий, проверен временем |
| 2D рендер | **SDL2_Renderer** (software) | Работает без GPU |
| 3D рендер | **OpenGL 2.0 / ES 2.0** через SDL2 | Минимальный общий знаменатель |
| Скрипты | **LuaJIT** | Быстрый, AI отлично генерирует Lua |
| Шрифты | **SDL2_ttf** | TrueType для ASCII-режима |
| Изображения | **SDL2_image** | PNG для тайлсетов |
| Звук | **SDL2_mixer** | OGG/WAV |
| Сборка | **Makefile** + CMake (для CI) | Простота |
| Контроль версий | **Git + GitHub** | Стандарт для open source |

---

## АРХИТЕКТУРА ДВИЖКА

### Общая схема

```
┌──────────────────────────────────────────────────────────┐
│                    LUA-КОНТЕНТ (моды)                     │
│  Мобы · Предметы · Квесты · Биомы · Рецепты · Текстуры  │
│  Способности · Диалоги · Эффекты · Лор · UI-виджеты     │
│                                                          │
│  ← Вайб-кодеры работают ТОЛЬКО здесь                     │
├──────────────────────────────────────────────────────────┤
│                    LUA API (биндинги)                     │
│  spawn() · damage() · create_item() · draw_sprite()      │
│  gen_texture() · play_sound() · show_dialog()            │
│  add_recipe() · set_tile() · register_biome()            │
├──────────────────────────────────────────────────────────┤
│                 ЯДРО ДВИЖКА (C11 + SDL2)                  │
│                                                          │
│  ┌─────────┐ ┌─────────┐ ┌──────────┐ ┌──────────────┐  │
│  │ Рендер  │ │  ECS    │ │ ProcGen  │ │ Процедурные  │  │
│  │ Система │ │ (сущн.) │ │ (мир)    │ │ текстуры     │  │
│  ├─────────┤ ├─────────┤ ├──────────┤ ├──────────────┤  │
│  │ ASCII   │ │Позиция  │ │BSP дандж │ │Perlin noise  │  │
│  │ 2D тайл │ │Здоровье │ │Биомы    │ │Voronoi       │  │
│  │ 2.5D изо│ │AI       │ │Ландшафт │ │Cellular auto │  │
│  │ 3D low  │ │Инвентарь│ │Лут      │ │L-systems     │  │
│  └─────────┘ └─────────┘ └──────────┘ └──────────────┘  │
│                                                          │
│  ┌─────────┐ ┌─────────┐ ┌──────────┐ ┌──────────────┐  │
│  │  Ввод   │ │  Звук   │ │ Физика   │ │ Сохранения   │  │
│  │ клав/тач│ │ музыка  │ │коллизии  │ │ JSON/binary  │  │
│  │ геймпад │ │ SFX     │ │движение  │ │ авто-сейв    │  │
│  └─────────┘ └─────────┘ └──────────┘ └──────────────┘  │
├──────────────────────────────────────────────────────────┤
│            SDL2 + OpenGL 2.0 + LuaJIT                    │
├──────────────────────────────────────────────────────────┤
│  Linux │ Windows │ macOS │ Android │ iOS │ Web (WASM)    │
└──────────────────────────────────────────────────────────┘
```

### Entity-Component-System (ECS)

Каждый объект в мире — сущность (entity) с набором компонентов:

```c
/* Компоненты — чистые данные, без логики */
typedef struct { int x, y, z; } PositionComponent;
typedef struct { int current, max; } HealthComponent;
typedef struct { char glyph; uint32_t color; int sprite_id; } RenderComponent;
typedef struct { int attack, defense, speed; } StatsComponent;
typedef struct { char script_path[128]; } ScriptComponent; /* → Lua */
typedef struct { int slots[MAX_INVENTORY]; int count; } InventoryComponent;
typedef struct { int faction_id; int hostility; } FactionComponent;

/* Сущность = ID + битовая маска компонентов */
typedef struct {
    uint32_t id;
    uint64_t mask;  /* какие компоненты есть */
} Entity;

/* Мир = массивы компонентов + массив сущностей */
typedef struct {
    Entity entities[MAX_ENTITIES];
    PositionComponent positions[MAX_ENTITIES];
    HealthComponent healths[MAX_ENTITIES];
    RenderComponent renders[MAX_ENTITIES];
    StatsComponent stats[MAX_ENTITIES];
    ScriptComponent scripts[MAX_ENTITIES];
    /* ... */
    int entity_count;
} World;
```

**Зачем ECS:**
- Cache-friendly (массивы, не указатели)
- Легко сериализовать (сохранения)
- Lua-скрипты добавляют/снимают компоненты динамически
- Системы обрабатывают пакетами (быстро даже на Celeron)

---

## РЕНДЕР-СИСТЕМА: 4 РЕЖИМА ИЗ ОДНОГО КОДА

Это ключевая архитектурная идея. Игровая логика не знает, как
объекты отрисовываются. Рендер — это абстракция с 4 бэкендами.

### Абстрактный интерфейс

```c
typedef enum {
    RENDER_ASCII,      /* Классический roguelike: @#.~ */
    RENDER_TILES_2D,   /* Спрайты сверху, а-ля Pokémon/Zelda */
    RENDER_ISO_25D,    /* Изометрия, а-ля Diablo 1 / Baldur's Gate */
    RENDER_LOWPOLY_3D, /* Низкополигональный 3D, а-ля PS1 */
} RenderMode;

typedef struct {
    RenderMode mode;
    void (*init)(int width, int height);
    void (*render_map)(World *world, Camera *cam);
    void (*render_entity)(Entity *e, World *world, Camera *cam);
    void (*render_ui)(UIState *ui);
    void (*render_frame)(void);
    void (*cleanup)(void);
} Renderer;

/* Глобальный рендерер — переключается в рантайме */
extern Renderer *g_renderer;
```

### Режим 1: ASCII (SDL2_ttf)
- Символы моноширинным шрифтом 16×16
- Стены=#, Пол=., Вода=~, Игрок=@
- SDL_Renderer (software) — без GPU вообще
- ~200+ FPS на Celeron
- **Идеально для разработки и тестирования**

### Режим 2: 2D тайлы (SDL2 + спрайты)
- PNG тайлсеты 16×16 или 32×32
- Подгружаемые тайлсеты (community-made)
- Слои: земля → объекты → существа → эффекты → UI
- SDL_Renderer или SDL2+OpenGL для блендинга
- ~120+ FPS на Celeron

### Режим 3: 2.5D изометрия (SDL2 + OpenGL 2.0)
- Изометрическая проекция (2:1)
- Спрайты или билборды в изо-пространстве
- Глубина через Z-сортировку
- Тени и освещение через шейдеры (OpenGL 2.0 GLSL 1.20)
- ~60+ FPS на Celeron

### Режим 4: Low-poly 3D (OpenGL 2.0 / ES 2.0)
- Минимум: OpenGL 2.0 + GLSL 1.20 (Sandy Bridge гарантирует)
- Эстетика: PS1 / early-2000s (low-poly + affine texturing)
- Меши < 200 полигонов на объект
- Процедурные текстуры (генерируются кодом!)
- Простое освещение: ambient + 1 directional light
- ~30-60 FPS на Celeron (зависит от сцены)

### Переключение в рантайме

Игрок выбирает режим в настройках. Игровая логика идентична —
меняется только визуализация. Это позволяет:
- Разрабатывать в ASCII (мгновенный фидбек)
- Играть в 2D тайлах (на слабом железе)
- Наслаждаться 3D (на более мощном)

---

## ПРОЦЕДУРНЫЕ ТЕКСТУРЫ: КОД ВМЕСТО КАРТИНОК

Это решение проблемы «красивые текстуры + маленький размер».
Текстура описывается не файлом PNG (тысячи КБ), а набором
параметров в Lua (~20 строк → ~500 байт). Движок генерирует
текстуру в RAM при загрузке и кэширует как SDL_Texture/GL texture.

### Как вайб-кодер создаёт текстуру

```lua
-- mods/ash_stone/textures.lua
register_texture("ash_stone", {
    size = 32,                    -- 32×32 пикселей
    base_color = {0.35, 0.32, 0.28},  -- серо-коричневый
    layers = {
        -- Слой 1: базовый шум (камень)
        {
            type = "perlin",
            scale = 4.0,
            intensity = 0.3,
            color_var = {0.05, 0.05, 0.03},
        },
        -- Слой 2: трещины
        {
            type = "voronoi",
            scale = 6.0,
            edge_width = 0.05,
            edge_color = {0.15, 0.12, 0.10},
        },
        -- Слой 3: пепельный налёт (сверху)
        {
            type = "perlin",
            scale = 2.0,
            intensity = 0.15,
            color = {0.55, 0.50, 0.45},
            blend = "overlay",
        },
    },
    -- Опциональные эффекты
    tiling = true,        -- бесшовная
    normal_map = true,    -- авто-генерация нормалей для 3D
    roughness = 0.7,      -- для PBR-подобного освещения
})
```

Вайб-кодер говорит AI: *«Создай текстуру ржавого металла для
Ashlands, 32×32, с царапинами и рыжими пятнами коррозии»* —
и получает готовый Lua-файл.

### Как движок генерирует текстуру

```c
/* texgen.c — процедурный генератор текстур */

/* Perlin noise (2D, референсная реализация) */
float perlin2d(float x, float y, int seed);

/* Voronoi / Worley noise */
float voronoi2d(float x, float y, int seed, float *edge_dist);

/* Генерация текстуры из Lua-описания */
SDL_Texture* texgen_create(SDL_Renderer *ren, TexParams *params) {
    uint32_t *pixels = malloc(params->size * params->size * 4);

    for (int y = 0; y < params->size; y++) {
        for (int x = 0; x < params->size; x++) {
            float r = params->base_color[0];
            float g = params->base_color[1];
            float b = params->base_color[2];

            /* Применяем каждый слой */
            for (int l = 0; l < params->layer_count; l++) {
                apply_layer(&r, &g, &b, x, y, &params->layers[l]);
            }

            pixels[y * params->size + x] = pack_rgba(r, g, b, 1.0f);
        }
    }

    SDL_Surface *surf = SDL_CreateRGBSurfaceFrom(
        pixels, params->size, params->size, 32,
        params->size * 4, 0xFF, 0xFF00, 0xFF0000, 0xFF000000
    );
    SDL_Texture *tex = SDL_CreateTextureFromSurface(ren, surf);
    SDL_FreeSurface(surf);
    free(pixels);
    return tex;
}
```

### Размер: код vs картинка

| Подход | Текстура 32×32 | 100 текстур | 1000 текстур |
|--------|---------------|-------------|-------------|
| PNG файлы | ~2 КБ | ~200 КБ | ~2 МБ |
| Lua-описания | ~0.5 КБ | ~50 КБ | ~500 КБ |
| Экономия | 75% | 75% | 75% |

Но главное не размер, а **вариативность**: из одного описания
движок может генерировать слегка разные варианты (через seed),
создавая естественный вид без повторов.

### Гибридный подход

Система поддерживает и процедурные текстуры, и обычные PNG:

```lua
-- Процедурная текстура (генерируется движком)
register_texture("ash_stone", { type = "procedural", ... })

-- Рисованная текстура (PNG-файл от художника)
register_texture("hero_portrait", { type = "file", path = "hero.png" })

-- Гибрид: рисованная база + процедурные детали
register_texture("mossy_brick", {
    type = "hybrid",
    base = "brick_base.png",     -- 16×16 нарисованная база
    layers = {
        { type = "perlin", color = {0.2, 0.5, 0.1}, ... },  -- мох
    },
})
```

Это значит: художники могут рисовать базовые спрайты,
а процедурные слои добавляют вариативность и детали.

---

## ПРОЦЕДУРНАЯ ГЕНЕРАЦИЯ МИРА

### Многоуровневая генерация

```
Слой 1: Мегакарта (весь мир, 1000×1000 чанков)
    → Определяет биомы, высоты, климат
    → Генерируется один раз при создании мира
    → Perlin noise + Voronoi для регионов

Слой 2: Чанки (каждый 64×64 тайлов)
    → Генерируется при приближении игрока
    → Содержит: ландшафт, ресурсы, точки интереса
    → Ленивая загрузка: только вокруг игрока

Слой 3: Данжены (процедурные подземелья)
    → BSP-деревья для комнат и коридоров
    → Lua-скрипты для уникального контента комнат
    → Боссы, загадки, лут-таблицы

Слой 4: Детали (заполнение)
    → Мобы по таблицам спавна биома
    → Лут по таблицам биома + уровень сложности
    → NPC, диалоги, квестовые маркеры
```

---

## LUA API ДЛЯ МОДОВ

### Основные функции (экспортируются из C в Lua)

```lua
-- === СУЩНОСТИ ===
entity_spawn(type_id, x, y)          -- создать сущность
entity_destroy(entity_id)            -- уничтожить
entity_get_pos(entity_id)            -- → x, y
entity_set_pos(entity_id, x, y)      -- телепортировать
entity_get_hp(entity_id)             -- → current, max
entity_damage(entity_id, amount)     -- нанести урон
entity_heal(entity_id, amount)       -- вылечить
entity_has_tag(entity_id, tag)       -- проверить тег
entity_add_tag(entity_id, tag)       -- добавить тег
entity_find_nearby(x, y, radius, tag) -- найти рядом

-- === КАРТА ===
map_get_tile(x, y)                   -- → tile_type
map_set_tile(x, y, tile_type)        -- изменить тайл
map_is_walkable(x, y)               -- → bool
map_line_of_sight(x1, y1, x2, y2)   -- → bool
map_pathfind(x1, y1, x2, y2)        -- → {путь}

-- === ИНВЕНТАРЬ ===
inventory_add(entity_id, item_id, count)
inventory_remove(entity_id, item_id, count)
inventory_has(entity_id, item_id)    -- → count
inventory_list(entity_id)            -- → {items}

-- === КРАФТ ===
register_recipe(recipe_def)          -- зарегистрировать рецепт
craft_can(entity_id, recipe_id)      -- → bool
craft_do(entity_id, recipe_id)       -- выполнить крафт

-- === UI И ДИАЛОГИ ===
ui_show_message(text)                -- показать текст
ui_show_dialog(npc_id, dialog_tree)  -- диалог с выбором
ui_show_menu(options)                -- меню
ui_log(text, color)                  -- в лог событий

-- === ЗВУК ===
sound_play(sound_id)                 -- звуковой эффект
music_play(track_id)                 -- фоновая музыка

-- === ТЕКСТУРЫ ===
register_texture(name, params)       -- процедурная текстура
texture_from_file(name, path)        -- из PNG

-- === РЕГИСТРАЦИЯ КОНТЕНТА ===
register_creature(creature_def)      -- зарегистрировать моба
register_item(item_def)              -- предмет
register_biome(biome_def)            -- биом
register_quest(quest_def)            -- квест
register_faction(faction_def)        -- фракция

-- === МИР ===
world_get_time()                     -- → hour, day, season
world_get_weather()                  -- → weather_type
world_is_night()                     -- → bool
```

### Пример: полноценный мод «Пепельный волк»

```lua
-- mods/ash_wolf/mod.lua
local mod = {}

-- Процедурная текстура меха
register_texture("ash_wolf_fur", {
    size = 32,
    base_color = {0.4, 0.38, 0.35},
    layers = {
        { type = "perlin", scale = 8, intensity = 0.2,
          color_var = {0.1, 0.08, 0.05} },
        { type = "cellular", scale = 12, intensity = 0.1,
          color = {0.3, 0.28, 0.25} },
    },
})

-- Определение существа
register_creature({
    id = "ash_wolf",
    name = "Пепельный волк",

    -- Рендер для разных режимов
    render = {
        ascii = { glyph = "w", color = 0xA09080 },
        tile_2d = { texture = "ash_wolf_fur", sprite_sheet = "ash_wolf_2d.png" },
        iso_25d = { sprite_sheet = "ash_wolf_iso.png", directions = 8 },
        lowpoly_3d = { mesh = "wolf_lowpoly.obj", texture = "ash_wolf_fur" },
    },

    stats = { hp = 35, attack = 8, defense = 3, speed = 7 },
    tags = { "animal", "pack", "nocturnal", "predator" },

    loot = {
        { item = "wolf_pelt", chance = 0.8 },
        { item = "ash_fang",  chance = 0.3 },
        { item = "raw_meat",  chance = 1.0 },
    },

    spawn = {
        biomes = { "ashland", "dead_forest", "ruins" },
        time = "night",   -- появляется ночью
        group = { min = 2, max = 5 }, -- стаями
        rarity = 0.4,
    },
})

-- AI-поведение
function mod.on_see(self, target)
    if entity_has_tag(target, "fire") then
        -- Волки боятся огня
        local tx, ty = entity_get_pos(target)
        local sx, sy = entity_get_pos(self.id)
        local dx = sx - tx
        local dy = sy - ty
        entity_set_pos(self.id, sx + dx, sy + dy)
        return
    end

    if entity_has_tag(target, "player") and world_is_night() then
        sound_play("wolf_howl")
        -- Привлекает волков в радиусе 15 тайлов
        local pack = entity_find_nearby(
            entity_get_pos(self.id), 15, "ash_wolf"
        )
        for _, wolf in ipairs(pack) do
            wolf:set_target(target)
        end
    end
end

function mod.on_turn(self)
    local pack = entity_find_nearby(
        entity_get_pos(self.id), 8, "ash_wolf"
    )

    if #pack >= 3 and self:can_see_player() then
        self:attack_target()
    else
        self:wander()
    end
end

function mod.on_death(self)
    ui_log("Пепельный волк испускает последний вой...", 0x808080)
    sound_play("wolf_death_howl")
    -- Оставшаяся стая становится агрессивнее
    local pack = entity_find_nearby(
        entity_get_pos(self.id), 12, "ash_wolf"
    )
    for _, wolf in ipairs(pack) do
        wolf.aggression = wolf.aggression + 0.3
    end
end

return mod
```

**Обрати внимание:** мод определяет рендер для ВСЕХ 4 режимов.
Если у вайб-кодера нет спрайтов — достаточно ASCII-версии,
остальные режимы используют fallback (процедурную текстуру
натянутую на дефолтный меш «четвероногое существо»).

---

## КРОССПЛАТФОРМЕННАЯ СБОРКА

### Makefile (разработка)
```makefile
CC = gcc
CFLAGS = -Wall -O2 -std=c11 \
         $(shell sdl2-config --cflags) \
         -I/usr/include/luajit-2.1
LDFLAGS = $(shell sdl2-config --libs) \
          -lSDL2_image -lSDL2_ttf -lSDL2_mixer \
          -lluajit-5.1 -lGL -lm

SRC = $(wildcard src/*.c)
OBJ = $(SRC:.c=.o)

ashlands: $(OBJ)
	$(CC) $(OBJ) -o $@ $(LDFLAGS)

# Веб-билд (для демо и Telegram Mini App)
web:
	emcc $(SRC) -o web/index.html \
		-s USE_SDL=2 -s USE_SDL_TTF=2 \
		-s USE_SDL_IMAGE=2 -s USE_SDL_MIXER=2 \
		-s ALLOW_MEMORY_GROWTH=1 -s MAX_WEBGL_VERSION=2 \
		-s FULL_ES2=1 \
		--preload-file assets/ --preload-file mods/ -O2

clean:
	rm -f src/*.o ashlands
	rm -rf web/
```

### Нативные платформы
- **Linux:** gcc + make (основная разработка)
- **Windows:** MinGW-w64 кросс-компиляция или GitHub Actions
- **macOS:** clang + make (GitHub Actions)
- **Android:** NDK + SDL2 android-project → .apk для Google Play
- **iOS:** Xcode + SDL2 iOS template (нужен Mac)
- **Web (WASM):** Emscripten (для демо, Telegram Mini App)

### GitHub Actions CI/CD
При каждом push собираются билды для:
Linux, Windows, Web. При тегах (релизах) — также Android.

---

## СТРУКТУРА ПРОЕКТА

```
ashlands/
├── src/                    # C-код ядра
│   ├── main.c              # Точка входа, игровой цикл
│   ├── engine.h/c          # Инициализация, конфиг, главный цикл
│   ├── ecs.h/c             # Entity-Component-System
│   ├── world.h/c           # Мир: чанки, биомы, время
│   │
│   ├── render/             # Рендер-система
│   │   ├── render.h        # Абстрактный интерфейс
│   │   ├── render_ascii.c  # ASCII-бэкенд
│   │   ├── render_tiles.c  # 2D тайловый бэкенд
│   │   ├── render_iso.c    # 2.5D изометрический
│   │   ├── render_3d.c     # Low-poly 3D (OpenGL 2.0)
│   │   └── camera.h/c      # Камера (работает во всех режимах)
│   │
│   ├── texgen/             # Процедурные текстуры
│   │   ├── texgen.h/c      # Генератор текстур
│   │   ├── noise.h/c       # Perlin, Simplex, Voronoi, Cellular
│   │   └── texcache.h/c    # Кэш сгенерированных текстур
│   │
│   ├── procgen/            # Процедурная генерация мира
│   │   ├── procgen.h       # Общий интерфейс
│   │   ├── dungeon.c       # BSP-данжены
│   │   ├── overworld.c     # Открытый мир
│   │   ├── biome.c         # Биомы
│   │   └── loot.c          # Лут-таблицы
│   │
│   ├── input.h/c           # Ввод: клавиатура, тач, геймпад
│   ├── audio.h/c           # Звук и музыка
│   ├── save.h/c            # Сохранение/загрузка
│   ├── lua_api.h/c         # Биндинги Lua
│   ├── combat.h/c          # Боевая система
│   ├── inventory.h/c       # Инвентарь и крафт
│   └── ui.h/c              # Игровой интерфейс
│
├── include/                # Общие заголовки
│   └── ashlands.h          # Глобальные типы и константы
│
├── mods/                   # Lua-контент (моды)
│   ├── core/               # Базовый контент (каноничный)
│   │   ├── creatures/
│   │   ├── items/
│   │   ├── biomes/
│   │   ├── quests/
│   │   ├── textures/
│   │   └── recipes/
│   └── community/          # Коммьюнити-моды
│
├── templates/              # Шаблоны для вайб-кодеров
│   ├── creature_template.lua
│   ├── item_template.lua
│   ├── biome_template.lua
│   ├── quest_template.lua
│   ├── texture_template.lua
│   └── README.md           # "Как создать свой мод"
│
├── assets/                 # Статические ассеты
│   ├── fonts/              # Моноширинные шрифты
│   ├── tilesets/           # PNG-тайлсеты (опционально)
│   ├── meshes/             # Low-poly .obj модели
│   ├── sounds/             # SFX (.wav)
│   └── music/              # Музыка (.ogg)
│
├── web/                    # WASM-билд (автогенерируется)
├── android/                # Android-проект (SDL2 template)
│
├── .github/workflows/      # CI/CD
│   └── build.yml
├── Makefile
├── CMakeLists.txt          # Для CI и сложных билдов
├── README.md
├── CONTRIBUTING.md         # Гайд для контрибьюторов
└── LICENSE                 # MIT
```

---

## ПОРЯДОК РАЗРАБОТКИ

### Фаза 1: Скелет (недели 1-2)
```
День 1:  main.c + engine.c     → Окно, игровой цикл, FPS
День 2:  render_ascii.c        → ASCII-рендер (шрифт + тайлы)
День 3:  input.c               → Движение @ по карте
День 4:  ecs.c                 → Базовые сущности
День 5:  map + procgen/dungeon → Процедурный данжен
День 6:  lua_api.c             → LuaJIT встроен, первые биндинги
День 7:  ТЕСТ                  → Ходишь по данжену, ASCII-рендер

День 8:  combat.c              → Удар, урон, смерть
День 9:  inventory.c           → Подобрать, использовать
День 10: save.c                → Сохранение/загрузка
День 11: Первый Lua-моб        → Крыса, управляемая Lua
День 12: texgen/noise.c        → Perlin noise, базовый генератор
День 13: render_tiles.c        → 2D-тайловый рендер
День 14: MVP ГОТОВ!            → Играбельно в ASCII и 2D
```

### Фаза 2: Рост (недели 3-6)
- render_iso.c (2.5D)
- Система биомов
- 20+ мобов через Lua
- Крафт v2
- Строительство
- Звук
- GitHub Actions CI

### Фаза 3: 3D и полировка (недели 7-12)
- render_3d.c (OpenGL 2.0)
- Процедурные текстуры в полном объёме
- Система квестов
- Фракции и NPC
- Android-билд
- Первый публичный релиз

---

## [ТЕКУЩИЙ СТАТУС]

> Обновляй эту секцию каждую сессию!

**Дата:** ___
**Фаза:** 0 (подготовка)
**Готовые файлы:** пока нет
**Текущая задача:** ___
**Проблемы:** ___

---

## ТРЕБОВАНИЯ К КОДУ

1. **Язык:** чистый C11 (НЕ C++)
2. **Стиль:** snake_case для функций и переменных
3. **Комментарии:** на русском (для тебя) или английском (для GitHub)
4. **Память:** никаких утечек. malloc → free. Используй arenas где можно
5. **Совместимость:** компилируется gcc и clang без ошибок и warnings
6. **Портабельность:** никаких системных вызовов (system(), fork(), popen())
7. **Файлы:** только через SDL_RWops или стандартный fopen
8. **OpenGL:** максимум 2.0 / GLSL 1.20 (Sandy Bridge минимум)
9. **Размер:** один .c файл ≤ 500 строк. Если больше — разбить
10. **Тесты:** каждый модуль должен компилироваться отдельно

---

*«Первая строка кода — это первый шаг по пеплу.
Последняя — это мир, в котором хотят жить.»*
