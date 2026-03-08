#include "quest.h"

#include "faction.h"
#include "item.h"
#include "lua_api.h"
#include "npc.h"
#include "world.h"
#include "lua_compat.h"
#include "../include/ashlands.h"

#include <string.h>

typedef struct {
    QuestDef defs[MAX_QUESTS];
    int active_stage[MAX_QUESTS];
    int progress[MAX_QUESTS][MAX_QUEST_OBJECTIVES];
    bool active[MAX_QUESTS];
    bool completed[MAX_QUESTS];
    int count;
} QuestState;

static QuestState s_quests;
static WorldState *s_quest_world = NULL;

static void quest_apply_rewards(int player_id, QuestStage *stage) {
    World *w = s_quest_world ? s_quest_world->ecs : NULL;

    for (int i = 0; i < stage->reward_count; i++) {
        QuestReward *reward = &stage->rewards[i];

        switch (reward->type) {
        case QUEST_REWARD_ITEM:
            if (w && player_id >= 0) {
                InventoryComponent *inv = entity_inventory(w, player_id);
                if (inv) {
                    for (int slot = 0; slot < MAX_INVENTORY_SLOTS; slot++) {
                        if (inv->slots[slot] < 0) {
                            item_spawn(w, reward->id, 0, 0, true, slot,
                                       MAX(reward->count, 1));
                            inv->count = MIN(inv->count + 1, MAX_INVENTORY_SLOTS);
                            break;
                        }
                    }
                }
            }
            break;
        case QUEST_REWARD_XP:
            if (w && player_id >= 0 && entity_has(w, player_id, COMP_STATS)) {
                StatsComponent *st = entity_stats(w, player_id);
                st->xp += reward->amount;
                while (st->xp >= st->level * 100) {
                    st->xp -= st->level * 100;
                    st->level++;
                    st->attack++;
                    st->defense++;
                }
            }
            break;
        case QUEST_REWARD_REPUTATION:
            faction_change_rep(player_id, reward->faction, reward->amount);
            break;
        default:
            break;
        }
    }
}

static int quest_find(const char *quest_id) {
    for (int i = 0; i < s_quests.count; i++) {
        if (strcmp(s_quests.defs[i].id, quest_id) == 0) {
            return i;
        }
    }
    return -1;
}

static void quest_parse_objectives(struct lua_State *L, int index, QuestStage *stage) {
    int count = (int)lua_objlen(L, index);

    stage->objective_count = MIN(count, MAX_QUEST_OBJECTIVES);
    for (int i = 0; i < stage->objective_count; i++) {
        QuestObjective *obj = &stage->objectives[i];
        const char *type_name;

        lua_rawgeti(L, index, i + 1);
        lua_getfield(L, -1, "type");
        type_name = luaL_optstring(L, -1, "");
        if (strcmp(type_name, "collect") == 0) {
            obj->type = QUEST_OBJECTIVE_COLLECT;
        } else if (strcmp(type_name, "talk_to") == 0) {
            obj->type = QUEST_OBJECTIVE_TALK_TO;
        }
        lua_pop(L, 1);
        lua_getfield(L, -1, "item");
        strncpy(obj->item, luaL_optstring(L, -1, ""), QUEST_ID_LEN - 1);
        lua_pop(L, 1);
        lua_getfield(L, -1, "npc");
        strncpy(obj->npc, luaL_optstring(L, -1, ""), QUEST_ID_LEN - 1);
        lua_pop(L, 1);
        lua_getfield(L, -1, "count");
        obj->count = (int)luaL_optinteger(L, -1, 1);
        lua_pop(L, 2);
    }
}

static void quest_parse_rewards(struct lua_State *L, int index, QuestStage *stage) {
    int count = (int)lua_objlen(L, index);

    stage->reward_count = MIN(count, MAX_QUEST_REWARDS);
    for (int i = 0; i < stage->reward_count; i++) {
        QuestReward *reward = &stage->rewards[i];
        const char *type_name;

        lua_rawgeti(L, index, i + 1);
        lua_getfield(L, -1, "type");
        type_name = luaL_optstring(L, -1, "");
        if (strcmp(type_name, "item") == 0) {
            reward->type = QUEST_REWARD_ITEM;
        } else if (strcmp(type_name, "xp") == 0) {
            reward->type = QUEST_REWARD_XP;
        } else if (strcmp(type_name, "reputation") == 0) {
            reward->type = QUEST_REWARD_REPUTATION;
        }
        lua_pop(L, 1);
        lua_getfield(L, -1, "id");
        strncpy(reward->id, luaL_optstring(L, -1, ""), QUEST_ID_LEN - 1);
        lua_pop(L, 1);
        lua_getfield(L, -1, "faction");
        strncpy(reward->faction, luaL_optstring(L, -1, ""), QUEST_ID_LEN - 1);
        lua_pop(L, 1);
        lua_getfield(L, -1, "amount");
        reward->amount = (int)luaL_optinteger(L, -1, 0);
        lua_pop(L, 1);
        lua_getfield(L, -1, "count");
        reward->count = (int)luaL_optinteger(L, -1, 1);
        lua_pop(L, 2);
    }
}

static void quest_parse_fail_conditions(struct lua_State *L, int index, QuestDef *def) {
    int count = (int)lua_objlen(L, index);

    def->fail_condition_count = MIN(count, MAX_QUEST_OBJECTIVES);
    for (int i = 0; i < def->fail_condition_count; i++) {
        QuestFailCondition *cond = &def->fail_conditions[i];
        const char *type_name;

        lua_rawgeti(L, index, i + 1);
        lua_getfield(L, -1, "type");
        type_name = luaL_optstring(L, -1, "");
        if (strcmp(type_name, "kill_npc") == 0) {
            cond->type = QUEST_OBJECTIVE_TALK_TO;
        }
        lua_pop(L, 1);
        lua_getfield(L, -1, "npc");
        strncpy(cond->npc, luaL_optstring(L, -1, ""), QUEST_ID_LEN - 1);
        lua_pop(L, 2);
    }
}

static int l_register_quest(struct lua_State *L) {
    QuestDef def;
    int index;

    memset(&def, 0, sizeof(def));
    luaL_checktype(L, 1, LUA_TTABLE);
    index = lua_absindex(L, 1);

    lua_getfield(L, index, "id");
    strncpy(def.id, luaL_checkstring(L, -1), QUEST_ID_LEN - 1);
    lua_pop(L, 1);
    lua_getfield(L, index, "name");
    strncpy(def.name, luaL_optstring(L, -1, def.id), QUEST_ID_LEN - 1);
    lua_pop(L, 1);
    lua_getfield(L, index, "description");
    strncpy(def.description, luaL_optstring(L, -1, ""), sizeof(def.description) - 1);
    lua_pop(L, 1);
    lua_getfield(L, index, "available_when");
    def.available_when_ref = lua_api_ref_function(L, -1);
    lua_pop(L, 1);
    lua_getfield(L, index, "fail_conditions");
    if (lua_istable(L, -1)) {
        quest_parse_fail_conditions(L, lua_gettop(L), &def);
    }
    lua_pop(L, 1);

    lua_getfield(L, index, "stages");
    if (lua_istable(L, -1)) {
        int count = (int)lua_objlen(L, -1);
        def.stage_count = MIN(count, MAX_QUEST_STAGES);
        for (int i = 0; i < def.stage_count; i++) {
            QuestStage *stage = &def.stages[i];
            const char *type_name;

            lua_rawgeti(L, -1, i + 1);
            lua_getfield(L, -1, "id");
            strncpy(stage->id, luaL_optstring(L, -1, "stage"), QUEST_ID_LEN - 1);
            lua_pop(L, 1);
            lua_getfield(L, -1, "text");
            strncpy(stage->text, luaL_optstring(L, -1, ""), sizeof(stage->text) - 1);
            lua_pop(L, 1);
            lua_getfield(L, -1, "type");
            type_name = luaL_optstring(L, -1, "");
            stage->is_end = strcmp(type_name, "end") == 0;
            lua_pop(L, 1);
            lua_getfield(L, -1, "on_complete");
            strncpy(stage->on_complete, luaL_optstring(L, -1, ""), QUEST_ID_LEN - 1);
            lua_pop(L, 1);
            lua_getfield(L, -1, "objectives");
            if (lua_istable(L, -1)) {
                quest_parse_objectives(L, lua_gettop(L), stage);
            }
            lua_pop(L, 1);
            lua_getfield(L, -1, "rewards");
            if (lua_istable(L, -1)) {
                quest_parse_rewards(L, lua_gettop(L), stage);
            }
            lua_pop(L, 2);
        }
    }
    lua_pop(L, 1);

    lua_pushboolean(L, quest_register(&def) >= 0);
    return 1;
}

void quest_init(void) {
    memset(&s_quests, 0, sizeof(s_quests));
}

void quest_shutdown(void) {
    memset(&s_quests, 0, sizeof(s_quests));
}

void quest_set_world(WorldState *ws) {
    s_quest_world = ws;
}

int quest_register(const QuestDef *def) {
    int index;

    if (!def || !def->id[0]) {
        return -1;
    }

    index = quest_find(def->id);
    if (index < 0) {
        if (s_quests.count >= MAX_QUESTS) {
            return -1;
        }
        index = s_quests.count++;
    }

    s_quests.defs[index] = *def;
    return index;
}

int quest_register_lua(struct lua_State *L) {
    lua_pushcfunction(L, l_register_quest);
    lua_setglobal(L, "register_quest");
    return 0;
}

int quest_count(void) {
    return s_quests.count;
}

const QuestDef *quest_get_by_index(int index) {
    if (index < 0 || index >= s_quests.count) {
        return NULL;
    }
    return &s_quests.defs[index];
}

bool quest_start(int player_id, const char *quest_id) {
    int index = quest_find(quest_id);

    (void)player_id;
    if (index < 0) {
        return false;
    }
    s_quests.active[index] = true;
    s_quests.completed[index] = false;
    s_quests.active_stage[index] = 0;
    memset(s_quests.progress[index], 0, sizeof(s_quests.progress[index]));
    return true;
}

const char *quest_get_stage(int player_id, const char *quest_id) {
    int index = quest_find(quest_id);

    (void)player_id;
    if (index < 0 || !s_quests.active[index]) {
        return NULL;
    }
    return s_quests.defs[index].stages[s_quests.active_stage[index]].id;
}

bool quest_advance(int player_id, const char *quest_id) {
    int index = quest_find(quest_id);

    (void)player_id;
    if (index < 0 || !s_quests.active[index]) {
        return false;
    }
    if (s_quests.active_stage[index] + 1 >= s_quests.defs[index].stage_count) {
        return quest_complete(player_id, quest_id);
    }
    s_quests.active_stage[index]++;
    memset(s_quests.progress[index], 0, sizeof(s_quests.progress[index]));
    return true;
}

bool quest_check_objectives(int player_id, const char *quest_id) {
    int index = quest_find(quest_id);
    QuestStage *stage;

    (void)player_id;
    if (index < 0 || !s_quests.active[index]) {
        return false;
    }

    stage = &s_quests.defs[index].stages[s_quests.active_stage[index]];
    for (int i = 0; i < stage->objective_count; i++) {
        int goal = MAX(stage->objectives[i].count, 1);
        if (s_quests.progress[index][i] < goal) {
            return false;
        }
    }
    return true;
}

bool quest_complete(int player_id, const char *quest_id) {
    int index = quest_find(quest_id);
    QuestStage *stage;

    (void)player_id;
    if (index < 0) {
        return false;
    }

    stage = &s_quests.defs[index].stages[s_quests.active_stage[index]];
    quest_apply_rewards(player_id, stage);

    s_quests.active[index] = false;
    s_quests.completed[index] = true;
    return true;
}

bool quest_fail(int player_id, const char *quest_id) {
    int index = quest_find(quest_id);

    (void)player_id;
    if (index < 0) {
        return false;
    }
    s_quests.active[index] = false;
    return true;
}

bool quest_active(const char *quest_id) {
    int index = quest_find(quest_id);
    return index >= 0 && s_quests.active[index];
}

bool quest_available(const char *quest_id) {
    int index = quest_find(quest_id);

    if (index < 0) {
        return false;
    }
    if (s_quests.active[index] || s_quests.completed[index]) {
        return false;
    }
    if (s_quests.defs[index].available_when_ref == LUA_NOREF) {
        return true;
    }
    return lua_api_call_ref_bool(s_quests.defs[index].available_when_ref);
}

int quest_list_active(int player_id, const char **out_ids, int max_out) {
    int count = 0;

    (void)player_id;
    if (!out_ids || max_out <= 0) {
        return 0;
    }

    for (int i = 0; i < s_quests.count && count < max_out; i++) {
        if (s_quests.active[i]) {
            out_ids[count++] = s_quests.defs[i].id;
        }
    }
    return count;
}

int quest_list_available(int player_id, const char **out_ids, int max_out) {
    int count = 0;

    (void)player_id;
    if (!out_ids || max_out <= 0) {
        return 0;
    }

    for (int i = 0; i < s_quests.count && count < max_out; i++) {
        if (quest_available(s_quests.defs[i].id)) {
            out_ids[count++] = s_quests.defs[i].id;
        }
    }
    return count;
}

void quest_get_state(const char *quest_id,
                     bool *active, bool *completed, int *stage) {
    int index = quest_find(quest_id);

    if (active) {
        *active = index >= 0 ? s_quests.active[index] : false;
    }
    if (completed) {
        *completed = index >= 0 ? s_quests.completed[index] : false;
    }
    if (stage) {
        *stage = index >= 0 ? s_quests.active_stage[index] : 0;
    }
}

void quest_set_state(const char *quest_id,
                     bool active, bool completed, int stage) {
    int index = quest_find(quest_id);

    if (index < 0) {
        return;
    }

    s_quests.active[index] = active;
    s_quests.completed[index] = completed;
    s_quests.active_stage[index] = CLAMP(stage, 0,
        MAX(s_quests.defs[index].stage_count - 1, 0));
}

void quest_get_progress(const char *quest_id, int *out_progress, int max_count) {
    int index = quest_find(quest_id);
    int count;

    if (index < 0 || !out_progress || max_count <= 0) {
        return;
    }

    count = MIN(max_count, MAX_QUEST_OBJECTIVES);
    for (int i = 0; i < count; i++) {
        out_progress[i] = s_quests.progress[index][i];
    }
}

void quest_set_progress(const char *quest_id, const int *progress, int count) {
    int index = quest_find(quest_id);

    if (index < 0 || !progress) {
        return;
    }

    count = MIN(count, MAX_QUEST_OBJECTIVES);
    for (int i = 0; i < count; i++) {
        s_quests.progress[index][i] = MAX(progress[i], 0);
    }
}

void quest_on_talk_to(const char *npc_id) {
    if (!npc_id) {
        return;
    }

    for (int qi = 0; qi < s_quests.count; qi++) {
        QuestStage *stage;

        if (!s_quests.active[qi]) {
            continue;
        }

        stage = &s_quests.defs[qi].stages[s_quests.active_stage[qi]];
        for (int oi = 0; oi < stage->objective_count; oi++) {
            if (stage->objectives[oi].type == QUEST_OBJECTIVE_TALK_TO &&
                strcmp(stage->objectives[oi].npc, npc_id) == 0) {
                s_quests.progress[qi][oi] = MAX(s_quests.progress[qi][oi], 1);
            }
        }

        if (quest_check_objectives(-1, s_quests.defs[qi].id)) {
            if (stage->is_end || !stage->on_complete[0]) {
                quest_complete(-1, s_quests.defs[qi].id);
            } else {
                quest_advance(-1, s_quests.defs[qi].id);
            }
        }
    }
}

void quest_on_collect(const char *item_id, int count) {
    if (!item_id || count <= 0) {
        return;
    }

    for (int qi = 0; qi < s_quests.count; qi++) {
        QuestStage *stage;

        if (!s_quests.active[qi]) {
            continue;
        }

        stage = &s_quests.defs[qi].stages[s_quests.active_stage[qi]];
        for (int oi = 0; oi < stage->objective_count; oi++) {
            if (stage->objectives[oi].type == QUEST_OBJECTIVE_COLLECT &&
                strcmp(stage->objectives[oi].item, item_id) == 0) {
                s_quests.progress[qi][oi] += count;
            }
        }

        if (quest_check_objectives(-1, s_quests.defs[qi].id)) {
            if (stage->is_end || !stage->on_complete[0]) {
                quest_complete(-1, s_quests.defs[qi].id);
            } else {
                quest_advance(-1, s_quests.defs[qi].id);
            }
        }
    }
}

void quest_on_kill_npc(const char *npc_id) {
    if (!npc_id) {
        return;
    }

    for (int qi = 0; qi < s_quests.count; qi++) {
        if (!s_quests.active[qi]) {
            continue;
        }
        for (int fi = 0; fi < s_quests.defs[qi].fail_condition_count; fi++) {
            QuestFailCondition *cond = &s_quests.defs[qi].fail_conditions[fi];
            if (cond->type == QUEST_OBJECTIVE_TALK_TO &&
                strcmp(cond->npc, npc_id) == 0) {
                quest_fail(-1, s_quests.defs[qi].id);
            }
        }
    }
}
