#ifndef ASHLANDS_QUEST_H
#define ASHLANDS_QUEST_H

#include <stdbool.h>

struct lua_State;
typedef struct WorldState WorldState;

#define MAX_QUESTS 32
#define MAX_QUEST_STAGES 8
#define MAX_QUEST_OBJECTIVES 8
#define MAX_QUEST_REWARDS 8
#define QUEST_ID_LEN 64

typedef enum {
    QUEST_OBJECTIVE_NONE = 0,
    QUEST_OBJECTIVE_COLLECT,
    QUEST_OBJECTIVE_TALK_TO,
} QuestObjectiveType;

typedef enum {
    QUEST_REWARD_NONE = 0,
    QUEST_REWARD_ITEM,
    QUEST_REWARD_XP,
    QUEST_REWARD_REPUTATION,
} QuestRewardType;

typedef struct {
    QuestObjectiveType type;
    char item[QUEST_ID_LEN];
    char npc[QUEST_ID_LEN];
    int count;
} QuestObjective;

typedef struct {
    QuestObjectiveType type;
    char npc[QUEST_ID_LEN];
} QuestFailCondition;

typedef struct {
    QuestRewardType type;
    char id[QUEST_ID_LEN];
    char faction[QUEST_ID_LEN];
    int amount;
    int count;
} QuestReward;

typedef struct {
    char id[QUEST_ID_LEN];
    char text[128];
    bool is_end;
    char on_complete[QUEST_ID_LEN];
    QuestObjective objectives[MAX_QUEST_OBJECTIVES];
    int objective_count;
    QuestReward rewards[MAX_QUEST_REWARDS];
    int reward_count;
} QuestStage;

typedef struct {
    char id[QUEST_ID_LEN];
    char name[QUEST_ID_LEN];
    char description[128];
    int available_when_ref;
    QuestFailCondition fail_conditions[MAX_QUEST_OBJECTIVES];
    int fail_condition_count;
    QuestStage stages[MAX_QUEST_STAGES];
    int stage_count;
} QuestDef;

void quest_init(void);
void quest_shutdown(void);
void quest_set_world(WorldState *ws);
int quest_register(const QuestDef *def);
int quest_register_lua(struct lua_State *L);
int quest_count(void);
const QuestDef *quest_get_by_index(int index);

bool quest_start(int player_id, const char *quest_id);
const char *quest_get_stage(int player_id, const char *quest_id);
bool quest_advance(int player_id, const char *quest_id);
bool quest_check_objectives(int player_id, const char *quest_id);
bool quest_complete(int player_id, const char *quest_id);
bool quest_fail(int player_id, const char *quest_id);
bool quest_active(const char *quest_id);
bool quest_available(const char *quest_id);
int quest_list_active(int player_id, const char **out_ids, int max_out);
int quest_list_available(int player_id, const char **out_ids, int max_out);
void quest_get_state(const char *quest_id,
                     bool *active, bool *completed, int *stage);
void quest_set_state(const char *quest_id,
                     bool active, bool completed, int stage);
void quest_get_progress(const char *quest_id, int *out_progress, int max_count);
void quest_set_progress(const char *quest_id, const int *progress, int count);
void quest_on_talk_to(const char *npc_id);
void quest_on_collect(const char *item_id, int count);
void quest_on_kill_npc(const char *npc_id);

#endif
