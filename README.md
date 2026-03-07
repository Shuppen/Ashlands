# Ashlands

> «В пепельных пустошах нет героев. Есть только те, кто ещё не сдался.»

**Ashlands** — open source roguelike-sandbox с элементами выживания.

Жанр: **Roguelike × Sandbox × Survival**
Аналоги: Dwarf Fortress (глубина) × Terraria (песочница) × Caves of Qud (лор) × Cataclysm:DDA (модульность)
Лицензия: **MIT**

---

## Ключевые особенности

- **4 режима рендера** из одной кодовой базы: ASCII → 2D тайлы → 2.5D изометрия → Low-poly 3D
- **Lua-моды** — весь контент (мобы, предметы, биомы, квесты) создаётся вайб-кодерами на Lua
- **Процедурные текстуры** — описания в Lua (~500 байт) вместо PNG-файлов
- **Работает на слабом железе** — Intel Celeron 1.6 ГГц, 2 ГБ RAM, нет GPU
- **Кроссплатформенность** — Linux, Windows, macOS, Android, Web (WASM)

---

## Быстрый старт

### Зависимости (Ubuntu/Debian)
```bash
sudo apt install libsdl2-dev libsdl2-ttf-dev libsdl2-image-dev \
                 libsdl2-mixer-dev libluajit-5.1-dev libgl1-mesa-dev
```

### Зависимости (Arch)
```bash
sudo pacman -S sdl2 sdl2_ttf sdl2_image sdl2_mixer luajit mesa
```

### Сборка и запуск
```bash
git clone https://github.com/[repo]/Ashlands
cd Ashlands
make
./ashlands
```

### Параметры запуска
```
./ashlands --help

  --fullscreen, -f   Полноэкранный режим
  --ascii            ASCII рендер (по умолчанию)
  --tiles            2D тайловый рендер
  --seed N           Сид мира (число)
  --width W          Ширина окна
  --height H         Высота окна
  --version, -v      Версия
```

---

## Управление

| Клавиши | Действие |
|---------|---------|
| Стрелки / WASD | Движение |
| Numpad 1-9 | Движение + диагонали |
| `.` / Пробел | Подождать |
| `E` | Взаимодействие |
| `G` | Подобрать предмет |
| `I` | Инвентарь |
| `Q` | Выход |
| `F11` | Полный экран |

---

## Создание модов

Ashlands создан для **вайб-кодеров**. Весь контент — на Lua:

```bash
cp templates/creature_template.lua mods/community/my_mod/my_creature.lua
# Отредактируй или попроси AI
```

Подробнее: [`templates/README.md`](templates/README.md)

---

## Архитектура

```
Движок (C11 + SDL2)
  ├── ECS — сущности и компоненты
  ├── Render — 4 бэкенда (ASCII/2D/ISO/3D)
  ├── ProcGen — BSP данжены, биомы
  ├── TexGen — процедурные текстуры
  └── LuaJIT — скрипты и API для модов

Контент (Lua)
  └── mods/core/ — базовые мобы, предметы, биомы, квесты
```

---

## Статус разработки

| Фаза | Статус | Что включено |
|------|--------|-------------|
| **Фаза 1** | ✅ В работе | Скелет: ASCII рендер, ECS, BSP данжен, движение, Lua API |
| Фаза 2 | Запланировано | 2D тайлы, биомы, 20+ мобов, крафт, звук |
| Фаза 3 | Запланировано | Low-poly 3D, квесты, фракции, Android, релиз |

---

## Участие в проекте

Читай [`CONTRIBUTING.md`](CONTRIBUTING.md).

Ты можешь создавать контент на Lua — **опыт программирования не обязателен**.
Просто используй шаблоны из `templates/` и AI-ассистента.

---

*Первая строка кода — это первый шаг по пеплу.
Последняя — это мир, в котором хотят жить.*
