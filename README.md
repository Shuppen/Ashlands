# Ashlands

[![Build](https://github.com/[repo]/Ashlands/actions/workflows/build.yml/badge.svg)](https://github.com/[repo]/Ashlands/actions/workflows/build.yml)
[![Pages](https://img.shields.io/badge/GitHub%20Pages-live-1f6feb)](https://[user].github.io/Ashlands/)
[![License: MIT](https://img.shields.io/badge/license-MIT-green.svg)](LICENSE)
[![Discord](https://img.shields.io/badge/Discord-community-5865F2)](#community)

> «В пепельных пустошах нет героев. Есть только те, кто ещё не сдался.»

**Ashlands** — open source roguelike-sandbox с элементами выживания. Это мир, где один и тот же код умеет рисоваться как ASCII, 2D тайлы, 2.5D изометрия и low-poly 3D. Контент, текстуры, квесты и NPC можно собирать на Lua без обязательного знания C.

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

## Играть прямо сейчас

- Web / GitHub Pages: `https://[user].github.io/Ashlands/`
- Telegram Mini App: использует ту же Web-сборку из `build-and-deploy.yml`

## Скриншоты и режимы

- `ASCII` — быстрый режим отладки и моддинга
- `2D tiles` — классический тайловый вид
- `2.5D ISO` — переходный режим для атмосферной навигации
- `Low-poly 3D` — fog, billboards, procedural textures, ретро-эстетика

Добавь сюда GIF или PNG из `assets/screenshots/` когда они будут готовы:
- `assets/screenshots/ascii.png`
- `assets/screenshots/tiles.png`
- `assets/screenshots/iso.png`
- `assets/screenshots/3d.png`

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

### Веб-билд и локальный предпросмотр
```bash
make web
cd build/web
python3 -m http.server 8080
```

`main`-push также запускает `.github/workflows/build-and-deploy.yml` и выкладывает веб-версию на GitHub Pages для Telegram Mini App.

### Параметры запуска
```
./ashlands --help

  --fullscreen, -f   Полноэкранный режим
  --ascii            ASCII рендер (по умолчанию)
  --tiles            2D тайловый рендер
  --iso              2.5D изометрический рендер
  --3d               Low-poly 3D рендер
  --seed N           Сид мира (число)
  --width W          Ширина окна
  --height H         Высота окна
  --version, -v      Версия
```

### Android APK (CI)
`.github/workflows/build.yml` теперь содержит `build-android` job для сборки SDL2/NDK APK и загрузки артефакта `ashlands-android`.

## Скачать

- Linux: artifact `ashlands-linux` из `build.yml`
- macOS: artifact `ashlands-macos` из `build.yml`
- Android: artifact `ashlands-android` из `build.yml`
- Web: GitHub Pages deployment из `build-and-deploy.yml`

---

## Управление

| Клавиши | Действие |
|---------|---------|
| Стрелки / WASD | Движение |
| Numpad 1-9 | Движение + диагонали |
| `.` / Пробел | Подождать |
| `E` | Взаимодействие |
| `1..8` | Выбор реплики в диалоге |
| `Esc` | Закрыть диалог / меню |
| `G` | Подобрать предмет |
| `R` | Бросить предмет |
| `I` | Инвентарь |
| `F5` | Сохранить игру |
| `F9` | Загрузить игру |
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

Стартовые Phase 3 Lua-примеры уже добавлены в:
- `mods/core/factions/factions.lua`
- `mods/core/quests/first_hunt.lua`
- `mods/core/npcs/grom_hunter.lua`

### Создай свой мод за 5 минут

```bash
mkdir -p mods/community/my_mod
cp templates/item_template.lua mods/community/my_mod/my_item.lua
```

Скажи AI: `Сделай из этого шаблона редкий пепельный амулет с Lua use_effect и ASCII glyph`.

## Минимальные требования

- CPU: Intel Celeron B815 класса или лучше
- RAM: 512 MB минимум, 2 GB рекомендовано
- GPU: OpenGL 2.0 / GLSL 1.20 или WebGL 1.0
- OS: Linux, Windows, macOS, Android, современный браузер

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
| Фаза 1 | ✅ Завершена | ASCII рендер, ECS, BSP данжен, движение, Lua API |
| Фаза 2 | ✅ Завершена | 2D/ISO foundation, биомы, предметы, мод-контент |
| Фаза 3 | ✅ Foundation готова | 3D, texgen, квесты, фракции, Android/CI, release scaffold |

---

## Участие в проекте

Читай [`CONTRIBUTING.md`](CONTRIBUTING.md).

Ты можешь создавать контент на Lua — **опыт программирования не обязателен**.
Просто используй шаблоны из `templates/` и AI-ассистента.

## Контрибьюторы

- Core maintainer: текущий автор проекта
- Community contributors: появятся через Issues, PR и будущий Mod Jam

## Community

- GitHub Issues: идеи, баги, моды
- Discord badge выше зарезервирован под community server
- Первый Mod Jam и public release scaffold уже готовы к наполнению

---

*Первая строка кода — это первый шаг по пеплу.
Последняя — это мир, в котором хотят жить.*
