#ifndef ASHLANDS_NPC_H
#define ASHLANDS_NPC_H

#include <stdbool.h>
#include <stdint.h>

#include "ecs.h"

struct lua_State;

#define MAX_NPCS 64
#define MAX_DIALOG_NODES 16
#define MAX_DIALOG_OPTIONS 8
#define NPC_ID_LEN 64

typedef struct {
    char text[128];
    char next[NPC_ID_LEN];
    int condition_ref;
    int effect_ref;
} DialogOption;

typedef struct {
    char id[NPC_ID_LEN];
    char text[256];
    DialogOption options[MAX_DIALOG_OPTIONS];
    int option_count;
} DialogNode;

typedef struct {
    char id[NPC_ID_LEN];
    char name[NPC_ID_LEN];
    char faction[NPC_ID_LEN];
    char ascii_glyph;
    uint32_t ascii_color;
    DialogNode nodes[MAX_DIALOG_NODES];
    int node_count;
} NpcDef;

void npc_init(void);
void npc_shutdown(void);
int npc_register(const NpcDef *def);
int npc_register_lua(struct lua_State *L);
int npc_spawn(World *w, const char *npc_id, int x, int y);

bool npc_start_dialog(int player_id, const char *npc_id);
bool npc_select_option(int player_id, int option_index);
bool npc_dialog_open(void);
void npc_close_dialog(void);
int npc_get_visible_options(int player_id, DialogOption *out_options, int max_out);
const DialogNode *dialog_get_current(int player_id);
const NpcDef *npc_find_def(const char *npc_id);
const char *npc_get_active_id(void);
const char *npc_get_active_node_id(void);
bool npc_restore_dialog(const char *npc_id, const char *node_id, bool open);
const char *npc_id_for_entity(int entity_id);
int npc_entity_for_id(const char *npc_id);

#endif
