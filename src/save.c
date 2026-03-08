#include "save.h"

#include "engine.h"
#include "faction.h"
#include "item.h"
#include "npc.h"
#include "quest.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SAVE_FORMAT_VERSION 2

static int save_progress_slots(void) {
    return 8;
}

static void save_reset_faction_state(World *w) {
    for (int i = 0; i < faction_count(); i++) {
        const FactionDef *def = faction_get_by_index(i);
        if (!def) {
            continue;
        }
        faction_set_rep(w->player_id, def->id, def->default_player_rep);
    }
}

static void save_reset_quest_state(void) {
    int zero[MAX_QUEST_OBJECTIVES] = {0};

    for (int i = 0; i < quest_count(); i++) {
        const QuestDef *def = quest_get_by_index(i);
        if (!def) {
            continue;
        }
        quest_set_state(def->id, false, false, 0);
        quest_set_progress(def->id, zero, MAX_QUEST_OBJECTIVES);
    }
}

static void save_clear_generic_nonplayer_entities(World *w) {
    for (int id = 0; id < MAX_ENTITIES; id++) {
        if (!entity_is_alive(w, id)) {
            continue;
        }
        if (entity_has(w, id, COMP_PLAYER)) {
            continue;
        }
        if (entity_has_tag(w, id, "npc")) {
            continue;
        }
        if (entity_has(w, id, COMP_ITEM)) {
            continue;
        }
        entity_destroy(w, id);
    }
}

static void save_clear_world_items(World *w) {
    for (int id = 0; id < MAX_ENTITIES; id++) {
        if (!entity_is_alive(w, id)) {
            continue;
        }
        if (!entity_has(w, id, COMP_ITEM)) {
            continue;
        }
        if (entity_has(w, id, COMP_PLAYER)) {
            continue;
        }
        entity_destroy(w, id);
    }
}

static void save_clear_npcs(World *w) {
    for (int id = 0; id < MAX_ENTITIES; id++) {
        if (!entity_is_alive(w, id)) {
            continue;
        }
        if (!entity_has_tag(w, id, "npc")) {
            continue;
        }
        entity_destroy(w, id);
    }
}

static void save_reset_player(World *w) {
    PositionComponent *pos = entity_pos(w, w->player_id);
    HealthComponent *hp = entity_health(w, w->player_id);
    StatsComponent *st = entity_stats(w, w->player_id);
    InventoryComponent *inv = entity_inventory(w, w->player_id);

    if (pos) {
        pos->x = 0;
        pos->y = 0;
        pos->z = 0;
    }
    if (hp) {
        hp->max = 100;
        hp->current = 100;
    }
    if (st) {
        st->attack = 5;
        st->defense = 3;
        st->speed = 5;
        st->level = 1;
        st->xp = 0;
    }
    if (inv) {
        for (int i = 0; i < MAX_INVENTORY_SLOTS; i++) {
            inv->slots[i] = -1;
        }
        inv->count = 0;
    }
}

static int save_restore_item(World *w,
                             const char *item_id,
                             const char *name,
                             int type,
                             int value,
                             int stack_size,
                             int stack_count,
                             int x,
                             int y,
                             bool into_inventory,
                             int slot) {
    int item_ent = item_spawn(w, item_id, x, y, into_inventory, slot, stack_count);
    ItemComponent *item;

    if (item_ent < 0) {
        return -1;
    }

    item = entity_item(w, item_ent);
    if (!item) {
        entity_destroy(w, item_ent);
        return -1;
    }

    if (!item->name[0] && name) {
        strncpy(item->name, name, ITEM_NAME_LEN - 1);
        item->name[ITEM_NAME_LEN - 1] = '\0';
    }
    if (item->type == 0) {
        item->type = type;
    }
    if (item->value == 0) {
        item->value = value;
    }
    if (item->stack_size <= 1) {
        item->stack_size = MAX(stack_size, 1);
    }
    item->stack_count = MAX(stack_count, 1);
    return item_ent;
}

static void save_strip_newline(char *s) {
    if (!s) {
        return;
    }
    for (size_t i = 0; s[i]; i++) {
        if (s[i] == '\n' || s[i] == '\r') {
            s[i] = '\0';
            return;
        }
    }
}

static void save_write_line(FILE *fp, const char *key, const char *value) {
    fprintf(fp, "%s=%s\n", key, value ? value : "");
}

static void save_write_entity_line(FILE *fp, const World *w, int id) {
    const Entity *e = &w->entities[id];
    const PositionComponent *pos = entity_has((World *)w, id, COMP_POSITION) ? &w->positions[id] : NULL;
    const HealthComponent *hp = entity_has((World *)w, id, COMP_HEALTH) ? &w->healths[id] : NULL;
    const StatsComponent *st = entity_has((World *)w, id, COMP_STATS) ? &w->stats[id] : NULL;
    char line[256];

    snprintf(line, sizeof(line), "%u %llu %d %d %d %d %d %d %d %d %d",
             e->id,
             (unsigned long long)e->mask,
             pos ? pos->x : 0,
             pos ? pos->y : 0,
             pos ? pos->z : 0,
             hp ? hp->current : 0,
             hp ? hp->max : 0,
             st ? st->attack : 0,
             st ? st->defense : 0,
             st ? st->speed : 0,
             st ? st->level : 0);
    save_write_line(fp, "entity", line);
}

bool save_game(const char *path, const AshEngineState *eng) {
    FILE *fp;
    char buf[256];
    char item_line[256];
    PositionComponent *player;
    InventoryComponent *inv;

    if (!path || !eng || !eng->world || !eng->world->ecs) {
        return false;
    }

    fp = fopen(path, "wb");
    if (!fp) {
        return false;
    }

    snprintf(buf, sizeof(buf), "%u", eng->world->seed);
    save_write_line(fp, "version", "2");
    snprintf(buf, sizeof(buf), "%u", eng->world->seed);
    save_write_line(fp, "seed", buf);
    snprintf(buf, sizeof(buf), "%d", (int)eng->world->weather);
    save_write_line(fp, "weather", buf);
    snprintf(buf, sizeof(buf), "%d %d %d %d",
             eng->world->time.hour, eng->world->time.day,
             eng->world->time.season, eng->world->time.tick);
    save_write_line(fp, "time", buf);
    snprintf(buf, sizeof(buf), "%d", eng->world->time.ticks_per_hour);
    save_write_line(fp, "ticks_per_hour", buf);

    player = entity_pos(eng->world->ecs, eng->world->ecs->player_id);
    if (player) {
        snprintf(buf, sizeof(buf), "%d %d %d", player->x, player->y, player->z);
        save_write_line(fp, "player_pos", buf);
    }
    if (entity_has(eng->world->ecs, eng->world->ecs->player_id, COMP_HEALTH)) {
        HealthComponent *hp = entity_health(eng->world->ecs, eng->world->ecs->player_id);
        snprintf(buf, sizeof(buf), "%d %d", hp->current, hp->max);
        save_write_line(fp, "player_health", buf);
    }
    if (entity_has(eng->world->ecs, eng->world->ecs->player_id, COMP_STATS)) {
        StatsComponent *st = entity_stats(eng->world->ecs, eng->world->ecs->player_id);
        snprintf(buf, sizeof(buf), "%d %d %d %d %d",
                 st->attack, st->defense, st->speed, st->level, st->xp);
        save_write_line(fp, "player_stats", buf);
    }

    inv = entity_inventory(eng->world->ecs, eng->world->ecs->player_id);
    if (inv) {
        snprintf(buf, sizeof(buf), "%d", inv->count);
        save_write_line(fp, "inventory_count", buf);
        for (int i = 0; i < MAX_INVENTORY_SLOTS; i++) {
            if (inv->slots[i] >= 0) {
                ItemComponent *item = entity_item(eng->world->ecs, inv->slots[i]);
                snprintf(item_line, sizeof(item_line), "%d %s %s %d %d %d %d",
                         i,
                         item ? item->id : "",
                         item ? item->name : "",
                         item ? item->type : 0,
                         item ? item->value : 0,
                         item ? item->stack_size : 1,
                         item ? item->stack_count : 1);
                save_write_line(fp, "inventory", item_line);
            }
        }
    }

    for (int id = 0; id < MAX_ENTITIES; id++) {
        ItemComponent *item;
        PositionComponent *pos;

        if (!entity_is_alive(eng->world->ecs, id)) {
            continue;
        }
        if (!entity_has(eng->world->ecs, id, COMP_ITEM | COMP_POSITION)) {
            continue;
        }

        item = entity_item(eng->world->ecs, id);
        pos = entity_pos(eng->world->ecs, id);
        if (!item || !pos) {
            continue;
        }

        snprintf(item_line, sizeof(item_line), "%s %s %d %d %d %d %d %d",
                 item->id,
                 item->name,
                 item->type,
                 item->value,
                 item->stack_size,
                 item->stack_count,
                 pos->x,
                 pos->y);
        save_write_line(fp, "world_item", item_line);
    }

    for (int i = 0; i < faction_count(); i++) {
        const FactionDef *def = faction_get_by_index(i);
        if (!def) {
            continue;
        }
        snprintf(buf, sizeof(buf), "%s %d", def->id,
                 faction_get_rep(eng->world->ecs->player_id, def->id));
        save_write_line(fp, "faction", buf);
    }

    for (int i = 0; i < MAX_ENTITIES; i++) {
        const char *npc_id;
        PositionComponent *pos;

        if (!entity_is_alive(eng->world->ecs, i)) {
            continue;
        }
        npc_id = npc_id_for_entity(i);
        pos = entity_pos(eng->world->ecs, i);
        if (!npc_id || !pos) {
            continue;
        }

        snprintf(buf, sizeof(buf), "%s %d %d %d",
                 npc_id, pos->x, pos->y,
                 eng->world->ecs->factions[i].faction_id);
        save_write_line(fp, "npc", buf);
    }

    for (int id = 0; id < MAX_ENTITIES; id++) {
        if (!entity_is_alive(eng->world->ecs, id)) {
            continue;
        }
        if (id == eng->world->ecs->player_id) {
            continue;
        }
        if (entity_has_tag(eng->world->ecs, id, "npc")) {
            continue;
        }
        if (entity_has(eng->world->ecs, id, COMP_ITEM)) {
            continue;
        }
        save_write_entity_line(fp, eng->world->ecs, id);
    }

    for (int i = 0; i < quest_count(); i++) {
        const QuestDef *def = quest_get_by_index(i);
        bool active;
        bool completed;
        int stage;
        int progress[MAX_QUEST_OBJECTIVES] = {0};

        if (!def) {
            continue;
        }

        quest_get_state(def->id, &active, &completed, &stage);
        quest_get_progress(def->id, progress, MAX_QUEST_OBJECTIVES);
        snprintf(buf, sizeof(buf), "%s %d %d %d %d %d %d %d %d %d %d %d",
                 def->id,
                 active ? 1 : 0,
                 completed ? 1 : 0,
                 stage,
                 progress[0], progress[1], progress[2], progress[3],
                 progress[4], progress[5], progress[6], progress[7]);
        save_write_line(fp, "quest", buf);
    }

    for (int y = 0; y < eng->world->map.height; y++) {
        for (int x = 0; x < eng->world->map.width; x++) {
            Tile *tile = &eng->world->map.tiles[x][y];
            snprintf(buf, sizeof(buf), "%d %d %d %d %d %d",
                     x, y,
                     (int)tile->type,
                     tile->explored ? 1 : 0,
                     tile->visible ? 1 : 0,
                     (int)tile->variant);
            save_write_line(fp, "tile", buf);
        }
    }

    snprintf(buf, sizeof(buf), "%s %s %d",
             npc_get_active_id() ? npc_get_active_id() : "",
             npc_get_active_node_id() ? npc_get_active_node_id() : "",
             npc_dialog_open() ? 1 : 0);
    save_write_line(fp, "dialog", buf);

    fclose(fp);
    return true;
}

bool load_game(const char *path, AshEngineState *eng) {
    FILE *fp;
    char line[256];
    InventoryComponent *inv;
    PositionComponent *player;
    HealthComponent *hp;
    StatsComponent *st;

    if (!path || !eng || !eng->world || !eng->world->ecs) {
        return false;
    }

    fp = fopen(path, "rb");
    if (!fp) {
        return false;
    }

    inv = entity_inventory(eng->world->ecs, eng->world->ecs->player_id);
    player = entity_pos(eng->world->ecs, eng->world->ecs->player_id);
    hp = entity_health(eng->world->ecs, eng->world->ecs->player_id);
    st = entity_stats(eng->world->ecs, eng->world->ecs->player_id);
    save_clear_world_items(eng->world->ecs);
    save_clear_npcs(eng->world->ecs);
    save_clear_generic_nonplayer_entities(eng->world->ecs);
    save_reset_player(eng->world->ecs);
    save_reset_faction_state(eng->world->ecs);
    save_reset_quest_state();
    npc_close_dialog();
    if (inv) {
        for (int i = 0; i < MAX_INVENTORY_SLOTS; i++) {
            if (inv->slots[i] >= 0) {
                entity_destroy(eng->world->ecs, inv->slots[i]);
            }
            inv->slots[i] = -1;
        }
        inv->count = 0;
    }

    while (fgets(line, sizeof(line), fp)) {
        char *eq = strchr(line, '=');
        if (!eq) {
            continue;
        }

        *eq++ = '\0';
        save_strip_newline(eq);
        if (strcmp(line, "seed") == 0) {
            eng->world->seed = (uint32_t)strtoul(eq, NULL, 10);
        } else if (strcmp(line, "version") == 0) {
            int version = atoi(eq);
            if (version > SAVE_FORMAT_VERSION) {
                fclose(fp);
                return false;
            }
        } else if (strcmp(line, "weather") == 0) {
            eng->world->weather = (WeatherType)CLAMP(atoi(eq), 0, WEATHER_COUNT - 1);
        } else if (strcmp(line, "time") == 0) {
            sscanf(eq, "%d %d %d %d",
                   &eng->world->time.hour,
                   &eng->world->time.day,
                   &eng->world->time.season,
                   &eng->world->time.tick);
        } else if (strcmp(line, "ticks_per_hour") == 0) {
            eng->world->time.ticks_per_hour = MAX(atoi(eq), 1);
        } else if (strcmp(line, "player_pos") == 0) {
            if (player) {
                sscanf(eq, "%d %d %d", &player->x, &player->y, &player->z);
            }
        } else if (strcmp(line, "player_health") == 0) {
            if (hp) {
                sscanf(eq, "%d %d", &hp->current, &hp->max);
                hp->current = CLAMP(hp->current, 0, hp->max);
            }
        } else if (strcmp(line, "player_stats") == 0) {
            if (st) {
                sscanf(eq, "%d %d %d %d %d",
                       &st->attack, &st->defense, &st->speed, &st->level, &st->xp);
            }
        } else if (strcmp(line, "inventory_count") == 0) {
            if (inv) {
                inv->count = CLAMP(atoi(eq), 0, MAX_INVENTORY_SLOTS);
            }
        } else if (strcmp(line, "inventory") == 0) {
            int slot = 0;
            char item_id[ITEM_NAME_LEN];
            char name[ITEM_NAME_LEN];
            int type = 0;
            int value = 0;
            int stack_size = 1;
            int stack_count = 1;

            if (inv && sscanf(eq, "%d %63s %63s %d %d %d %d",
                              &slot, item_id, name, &type, &value,
                              &stack_size, &stack_count) == 7) {
                if (!item_exists(item_id)) {
                    continue;
                }
                save_restore_item(eng->world->ecs,
                                  item_id,
                                  name,
                                  type,
                                  value,
                                  stack_size,
                                  stack_count,
                                  0,
                                  0,
                                  true,
                                  slot);
            }
        } else if (strcmp(line, "faction") == 0) {
            char id[QUEST_ID_LEN];
            int rep = 0;
            if (sscanf(eq, "%63s %d", id, &rep) == 2) {
                faction_set_rep(eng->world->ecs->player_id, id, rep);
            }
        } else if (strcmp(line, "npc") == 0) {
            char npc_id[NPC_ID_LEN];
            int x = 0;
            int y = 0;
            int faction_id = 0;

            if (sscanf(eq, "%63s %d %d %d", npc_id, &x, &y, &faction_id) == 4) {
                int entity_id = npc_spawn(eng->world->ecs, npc_id, x, y);
                if (entity_id >= 0) {
                    eng->world->ecs->factions[entity_id].faction_id = faction_id;
                }
            }
        } else if (strcmp(line, "entity") == 0) {
            unsigned int saved_id = 0;
            unsigned long long mask = 0;
            int x = 0, y = 0, z = 0;
            int hp_cur = 0, hp_max = 0;
            int atk = 0, def = 0, spd = 0, lvl = 0;
            int entity_id;

            if (sscanf(eq, "%u %llu %d %d %d %d %d %d %d %d %d",
                       &saved_id, &mask, &x, &y, &z,
                       &hp_cur, &hp_max, &atk, &def, &spd, &lvl) == 11) {
                entity_id = entity_create(eng->world->ecs);
                if (entity_id >= 0) {
                    entity_add_comp(eng->world->ecs, entity_id, (uint64_t)mask);
                    if (entity_has(eng->world->ecs, entity_id, COMP_POSITION)) {
                        eng->world->ecs->positions[entity_id].x = x;
                        eng->world->ecs->positions[entity_id].y = y;
                        eng->world->ecs->positions[entity_id].z = z;
                    }
                    if (entity_has(eng->world->ecs, entity_id, COMP_HEALTH)) {
                        eng->world->ecs->healths[entity_id].current = hp_cur;
                        eng->world->ecs->healths[entity_id].max = hp_max;
                    }
                    if (entity_has(eng->world->ecs, entity_id, COMP_STATS)) {
                        eng->world->ecs->stats[entity_id].attack = atk;
                        eng->world->ecs->stats[entity_id].defense = def;
                        eng->world->ecs->stats[entity_id].speed = spd;
                        eng->world->ecs->stats[entity_id].level = lvl;
                    }
                }
            }
        } else if (strcmp(line, "quest") == 0) {
            char id[QUEST_ID_LEN];
            int active = 0;
            int completed = 0;
            int stage = 0;
            int progress[MAX_QUEST_OBJECTIVES] = {0};
            int count;

            count = sscanf(eq, "%63s %d %d %d %d %d %d %d %d %d %d %d",
                           id, &active, &completed, &stage,
                           &progress[0], &progress[1], &progress[2], &progress[3],
                           &progress[4], &progress[5], &progress[6], &progress[7]);
            if (count >= 4) {
                quest_set_state(id, active != 0, completed != 0, stage);
                quest_set_progress(id, progress, save_progress_slots());
            }
        } else if (strcmp(line, "tile") == 0) {
            int x = 0, y = 0, type = 0, explored = 0, visible = 0, variant = 0;
            if (sscanf(eq, "%d %d %d %d %d %d",
                       &x, &y, &type, &explored, &visible, &variant) == 6) {
                Tile *tile = map_tile(&eng->world->map, x, y);
                if (tile) {
                    tile->type = (TileType)CLAMP(type, 0, TILE_COUNT - 1);
                    tile->explored = explored != 0;
                    tile->visible = visible != 0;
                    tile->variant = (uint8_t)variant;
                }
            }
        } else if (strcmp(line, "world_item") == 0) {
            char item_id[ITEM_NAME_LEN];
            char name[ITEM_NAME_LEN];
            int type = 0;
            int value = 0;
            int stack_size = 1;
            int stack_count = 1;
            int x = 0;
            int y = 0;

            if (sscanf(eq, "%63s %63s %d %d %d %d %d %d",
                       item_id, name, &type, &value,
                       &stack_size, &stack_count, &x, &y) == 8) {
                if (!item_exists(item_id)) {
                    continue;
                }
                save_restore_item(eng->world->ecs,
                                  item_id,
                                  name,
                                  type,
                                  value,
                                  stack_size,
                                  stack_count,
                                  x,
                                  y,
                                  false,
                                  -1);
            }
        } else if (strcmp(line, "dialog") == 0) {
            char npc_id[NPC_ID_LEN] = {0};
            char node_id[NPC_ID_LEN] = {0};
            int open = 0;

            if (sscanf(eq, "%63s %63s %d", npc_id, node_id, &open) >= 1) {
                if (npc_id[0]) {
                    npc_restore_dialog(npc_id,
                                       node_id[0] ? node_id : NULL,
                                       open != 0);
                } else {
                    npc_close_dialog();
                }
            }
        }
    }

    fclose(fp);
    return true;
}
