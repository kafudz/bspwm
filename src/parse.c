#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <errno.h>
#include <limits.h>
#include "parse.h"

#define LOOKUP_TABLE(name, type) \
static const struct { const char *key; type value; } name##_table[] =

#define BINARY_SEARCH_PARSER(name, type) \
bool parse_##name(char *s, type *v) { \
    if (s == NULL || v == NULL) return false; \
    int left = 0, right = (int)(sizeof(name##_table) / sizeof(name##_table[0])) - 1; \
    while (left <= right) { \
        int mid = left + (right - left) / 2; \
        int cmp = strcmp(s, name##_table[mid].key); \
        if (cmp == 0) { *v = name##_table[mid].value; return true; } \
        if (cmp > 0) left = mid + 1; \
        else right = mid - 1; \
    } \
    return false; \
}

LOOKUP_TABLE(split_type, split_type_t) {
    {"horizontal", TYPE_HORIZONTAL}, {"vertical", TYPE_VERTICAL}
};
LOOKUP_TABLE(split_mode, split_mode_t) {
    {"automatic", MODE_AUTOMATIC}, {"vertical", MODE_MANUAL}
};
LOOKUP_TABLE(layout, layout_t) {
    {"monocle", LAYOUT_MONOCLE}, {"tiled", LAYOUT_TILED}
};
LOOKUP_TABLE(client_state, client_state_t) {
    {"floating", STATE_FLOATING}, {"fullscreen", STATE_FULLSCREEN},
    {"pseudo_tiled", STATE_PSEUDO_TILED}, {"tiled", STATE_TILED}
};
LOOKUP_TABLE(stack_layer, stack_layer_t) {
    {"above", LAYER_ABOVE}, {"below", LAYER_BELOW}, {"normal", LAYER_NORMAL}
};
LOOKUP_TABLE(snap_zone, snap_zone_t) {
    {"bottom", SNAP_BOTTOM}, {"bottom_left", SNAP_BOTTOM_LEFT},
    {"bottom_right", SNAP_BOTTOM_RIGHT}, {"left", SNAP_LEFT},
    {"maximize", SNAP_MAXIMIZE}, {"right", SNAP_RIGHT}, {"top", SNAP_TOP},
    {"top_left", SNAP_TOP_LEFT}, {"top_right", SNAP_TOP_RIGHT}
};
LOOKUP_TABLE(direction, direction_t) {
    {"east", DIR_EAST}, {"north", DIR_NORTH}, {"south", DIR_SOUTH}, {"west", DIR_WEST}
};
LOOKUP_TABLE(cycle_direction, cycle_dir_t) {
    {"next", CYCLE_NEXT}, {"prev", CYCLE_PREV}
};
LOOKUP_TABLE(circulate_direction, circulate_dir_t) {
    {"backward", CIRCULATE_BACKWARD}, {"forward", CIRCULATE_FORWARD}
};
LOOKUP_TABLE(history_direction, history_dir_t) {
    {"newer", HISTORY_NEWER}, {"older", HISTORY_OLDER}
};
LOOKUP_TABLE(flip, flip_t) {
    {"horizontal", FLIP_HORIZONTAL}, {"vertical", FLIP_VERTICAL}
};
LOOKUP_TABLE(resize_handle, resize_handle_t) {
    {"bottom", HANDLE_BOTTOM}, {"bottom_left", HANDLE_BOTTOM_LEFT},
    {"bottom_right", HANDLE_BOTTOM_RIGHT}, {"left", HANDLE_LEFT},
    {"right", HANDLE_RIGHT}, {"top", HANDLE_TOP}, {"top_left", HANDLE_TOP_LEFT},
    {"top_right", HANDLE_TOP_RIGHT}
};
LOOKUP_TABLE(pointer_action, pointer_action_t) {
    {"focus", ACTION_FOCUS}, {"move", ACTION_MOVE}, {"none", ACTION_NONE},
    {"resize_corner", ACTION_RESIZE_CORNER}, {"resize_side", ACTION_RESIZE_SIDE}
};
LOOKUP_TABLE(child_polarity, child_polarity_t) {
    {"first_child", FIRST_CHILD}, {"second_child", SECOND_CHILD}
};
LOOKUP_TABLE(automatic_scheme, automatic_scheme_t) {
    {"alternate", SCHEME_ALTERNATE}, {"longest_side", SCHEME_LONGEST_SIDE},
    {"spiral", SCHEME_SPIRAL}
};
LOOKUP_TABLE(tightness, tightness_t) {
    {"high", TIGHTNESS_HIGH}, {"low", TIGHTNESS_LOW}
};

BINARY_SEARCH_PARSER(split_type, split_type_t)
BINARY_SEARCH_PARSER(split_mode, split_mode_t)
BINARY_SEARCH_PARSER(layout, layout_t)
BINARY_SEARCH_PARSER(client_state, client_state_t)
BINARY_SEARCH_PARSER(stack_layer, stack_layer_t)
BINARY_SEARCH_PARSER(snap_zone, snap_zone_t)
BINARY_SEARCH_PARSER(direction, direction_t)
BINARY_SEARCH_PARSER(cycle_direction, cycle_dir_t)
BINARY_SEARCH_PARSER(circulate_direction, circulate_dir_t)
BINARY_SEARCH_PARSER(history_direction, history_dir_t)
BINARY_SEARCH_PARSER(flip, flip_t)
BINARY_SEARCH_PARSER(resize_handle, resize_handle_t)
BINARY_SEARCH_PARSER(pointer_action, pointer_action_t)
BINARY_SEARCH_PARSER(child_polarity, child_polarity_t)
BINARY_SEARCH_PARSER(automatic_scheme, automatic_scheme_t)
BINARY_SEARCH_PARSER(tightness, tightness_t)

bool parse_bool(char *value, bool *b)
{
    if (value == NULL || b == NULL) return false;
    if (strcmp(value, "true") == 0 || strcmp(value, "on") == 0) {
        *b = true;
        return true;
    }
    if (strcmp(value, "false") == 0 || strcmp(value, "off") == 0) {
        *b = false;
        return true;
    }
    return false;
}

bool parse_honor_size_hints_mode(char *s, honor_size_hints_mode_t *a)
{
    bool b;
    if (s == NULL || a == NULL) return false;
    if (parse_bool(s, &b)) {
        *a = b ? HONOR_SIZE_HINTS_YES : HONOR_SIZE_HINTS_NO;
        return true;
    }
    if (strcmp(s, "floating") == 0) {
        *a = HONOR_SIZE_HINTS_FLOATING;
        return true;
    }
    if (strcmp(s, "tiled") == 0) {
        *a = HONOR_SIZE_HINTS_TILED;
        return true;
    }
    return false;
}

bool parse_state_transition(char *s, state_transition_t *m)
{
    if (s == NULL || m == NULL) return false;
    if (strcmp(s, "none") == 0) { *m = 0; return true; }
    if (strcmp(s, "all") == 0) {
        *m = STATE_TRANSITION_ENTER | STATE_TRANSITION_EXIT;
        return true;
    }

    state_transition_t result = 0;
    const char *p = s;
    while (*p != '\0') {
        const char *end = strchr(p, ',');
        size_t len = end == NULL ? strlen(p) : (size_t)(end - p);
        if (len == 5 && strncmp(p, "enter", len) == 0) {
            result |= STATE_TRANSITION_ENTER;
        } else if (len == 4 && strncmp(p, "exit", len) == 0) {
            result |= STATE_TRANSITION_EXIT;
        } else {
            return false;
        }
        if (end == NULL) break;
        p = end + 1;
        if (*p == '\0') return false;
    }
    if (result == 0) return false;
    *m = result;
    return true;
}

static const bool valid_degrees[360] = {
    [0] = true, [90] = true, [180] = true, [270] = true
};

bool parse_degree(char *s, int *d)
{
    char *end;
    if (s == NULL || d == NULL) return false;
    errno = 0;
    long parsed = strtol(s, &end, 10);
    if (errno != 0 || end == s || *end != '\0') return false;
    int i = (int)(((parsed % 360) + 360) % 360);
    if (!valid_degrees[i]) return false;
    *d = i;
    return true;
}

bool parse_id(char *s, uint32_t *id)
{
    char *end;
    if (s == NULL || id == NULL) return false;
    errno = 0;
    long v = strtol(s, &end, 0);
    if (errno != 0 || end == s || *end != '\0' || v < 0 || (unsigned long)v > UINT32_MAX)
        return false;
    *id = (uint32_t)v;
    return true;
}

/* The whole string must be an integer representable as int and in [min, max]. */
bool parse_int(const char *s, long min, long max, int *v)
{
    char *end;
    if (s == NULL || v == NULL || min > max) return false;
    errno = 0;
    long parsed = strtol(s, &end, 0);
    if (errno != 0 || end == s || *end != '\0' || parsed < min || parsed > max ||
        parsed < INT_MIN || parsed > INT_MAX)
        return false;
    *v = (int)parsed;
    return true;
}

bool parse_double(const char *s, double min, double max, double *v)
{
    char *end;
    if (s == NULL || v == NULL || min > max) return false;
    errno = 0;
    double parsed = strtod(s, &end);
    if (errno != 0 || end == s || *end != '\0' || !(parsed >= min && parsed <= max))
        return false;
    *v = parsed;
    return true;
}

bool parse_bool_declaration(char *s, char **key, bool *value, alter_state_t *state)
{
    if (s == NULL || key == NULL || value == NULL || state == NULL || *s == '\0')
        return false;
    char *equal = strchr(s, '=');
    if (equal == NULL) {
        *key = s;
        *state = ALTER_TOGGLE;
        return true;
    }
    if (equal == s || equal[1] == '\0' || strchr(equal + 1, '=') != NULL)
        return false;
    *equal = '\0';
    if (!parse_bool(equal + 1, value)) {
        *equal = '=';
        return false;
    }
    *key = s;
    *state = ALTER_SET;
    return true;
}

bool parse_index(char *s, uint16_t *idx)
{
    if (s == NULL || idx == NULL || *s != '^') return false;
    char *end;
    errno = 0;
    unsigned long val = strtoul(s + 1, &end, 10);
    if (errno != 0 || end == s + 1 || *end != '\0' || val > UINT16_MAX)
        return false;
    *idx = (uint16_t)val;
    return true;
}

bool parse_rectangle(char *s, bspwm_rect_t *r)
{
    char *end;
    if (s == NULL || r == NULL) return false;
    errno = 0;
    unsigned long w = strtoul(s, &end, 10);
    if (errno != 0 || end == s || *end != 'x' || w == 0 || w > UINT16_MAX) return false;
    s = end + 1;
    errno = 0;
    unsigned long h = strtoul(s, &end, 10);
    if (errno != 0 || end == s || *end != '+' || h == 0 || h > UINT16_MAX) return false;
    s = end + 1;
    errno = 0;
    long x = strtol(s, &end, 10);
    if (errno != 0 || end == s || *end != '+' || x < INT16_MIN || x > INT16_MAX) return false;
    s = end + 1;
    errno = 0;
    long y = strtol(s, &end, 10);
    if (errno != 0 || end == s || *end != '\0' || y < INT16_MIN || y > INT16_MAX) return false;
    *r = (bspwm_rect_t){(int16_t)x, (int16_t)y, (uint16_t)w, (uint16_t)h};
    return true;
}

static const struct { const char *key; uint16_t mask; } modifier_table[] = {
    {"control", BSP_MOD_MASK_CONTROL}, {"lock", BSP_MOD_MASK_LOCK},
    {"mod1", BSP_MOD_MASK_1}, {"mod2", BSP_MOD_MASK_2},
    {"mod3", BSP_MOD_MASK_3}, {"mod4", BSP_MOD_MASK_4},
    {"mod5", BSP_MOD_MASK_5}, {"shift", BSP_MOD_MASK_SHIFT}
};

bool parse_modifier_mask(char *s, uint16_t *m)
{
    if (s == NULL || m == NULL) return false;
    int left = 0, right = (int)(sizeof(modifier_table) / sizeof(modifier_table[0])) - 1;
    while (left <= right) {
        int mid = left + (right - left) / 2;
        int cmp = strcmp(s, modifier_table[mid].key);
        if (cmp == 0) { *m = modifier_table[mid].mask; return true; }
        if (cmp > 0) left = mid + 1;
        else right = mid - 1;
    }
    return false;
}

bool parse_button_index(char *s, int8_t *b)
{
    if (s == NULL || b == NULL) return false;
    if (strcmp(s, "any") == 0) *b = BSP_BUTTON_ANY;
    else if (strcmp(s, "button1") == 0) *b = 1;
    else if (strcmp(s, "button2") == 0) *b = 2;
    else if (strcmp(s, "button3") == 0) *b = 3;
    else if (strcmp(s, "none") == 0) *b = -1;
    else return false;
    return true;
}

static const struct { const char *name; subscriber_mask_t mask; } subscriber_table[] = {
    {"all", SBSC_MASK_ALL}, {"desktop", SBSC_MASK_DESKTOP},
    {"desktop_activate", SBSC_MASK_DESKTOP_ACTIVATE}, {"desktop_add", SBSC_MASK_DESKTOP_ADD},
    {"desktop_focus", SBSC_MASK_DESKTOP_FOCUS}, {"desktop_layout", SBSC_MASK_DESKTOP_LAYOUT},
    {"desktop_remove", SBSC_MASK_DESKTOP_REMOVE}, {"desktop_rename", SBSC_MASK_DESKTOP_RENAME},
    {"desktop_swap", SBSC_MASK_DESKTOP_SWAP}, {"desktop_transfer", SBSC_MASK_DESKTOP_TRANSFER},
    {"monitor", SBSC_MASK_MONITOR}, {"monitor_add", SBSC_MASK_MONITOR_ADD},
    {"monitor_focus", SBSC_MASK_MONITOR_FOCUS}, {"monitor_geometry", SBSC_MASK_MONITOR_GEOMETRY},
    {"monitor_remove", SBSC_MASK_MONITOR_REMOVE}, {"monitor_rename", SBSC_MASK_MONITOR_RENAME},
    {"monitor_swap", SBSC_MASK_MONITOR_SWAP}, {"node", SBSC_MASK_NODE},
    {"node_activate", SBSC_MASK_NODE_ACTIVATE}, {"node_add", SBSC_MASK_NODE_ADD},
    {"node_flag", SBSC_MASK_NODE_FLAG}, {"node_focus", SBSC_MASK_NODE_FOCUS},
    {"node_geometry", SBSC_MASK_NODE_GEOMETRY}, {"node_layer", SBSC_MASK_NODE_LAYER},
    {"node_presel", SBSC_MASK_NODE_PRESEL}, {"node_remove", SBSC_MASK_NODE_REMOVE},
    {"node_stack", SBSC_MASK_NODE_STACK}, {"node_state", SBSC_MASK_NODE_STATE},
    {"node_swap", SBSC_MASK_NODE_SWAP}, {"node_transfer", SBSC_MASK_NODE_TRANSFER},
    {"pointer_action", SBSC_MASK_POINTER_ACTION}, {"report", SBSC_MASK_REPORT}
};

bool parse_subscriber_mask(char *s, subscriber_mask_t *mask)
{
    if (s == NULL || mask == NULL) return false;
    int left = 0, right = (int)(sizeof(subscriber_table) / sizeof(subscriber_table[0])) - 1;
    while (left <= right) {
        int mid = left + (right - left) / 2;
        int cmp = strcmp(s, subscriber_table[mid].name);
        if (cmp == 0) { *mask = subscriber_table[mid].mask; return true; }
        if (cmp > 0) left = mid + 1;
        else right = mid - 1;
    }
    return false;
}

#define PARSE_MODIFIER(field) \
    if (strcmp(#field, tok) == 0) { sel->field = OPTION_TRUE; } \
    else if (tok[0] == '!' && strcmp(#field, tok + 1) == 0) { sel->field = OPTION_FALSE; }

bool parse_monitor_modifiers(char *desc, monitor_select_t *sel)
{
    if (desc == NULL || sel == NULL) return false;
    char *tok;
    while ((tok = strrchr(desc, CAT_CHR)) != NULL) {
        *tok++ = '\0';
        if (strcmp("occupied", tok) == 0) sel->occupied = OPTION_TRUE;
        else if (strcmp("!occupied", tok) == 0) sel->occupied = OPTION_FALSE;
        else PARSE_MODIFIER(focused)
        else return false;
    }
    return true;
}

bool parse_desktop_modifiers(char *desc, desktop_select_t *sel)
{
    if (desc == NULL || sel == NULL) return false;
    char *tok;
    while ((tok = strrchr(desc, CAT_CHR)) != NULL) {
        *tok++ = '\0';
        if (strcmp("occupied", tok) == 0) sel->occupied = OPTION_TRUE;
        else if (strcmp("!occupied", tok) == 0) sel->occupied = OPTION_FALSE;
        else PARSE_MODIFIER(focused)
        else PARSE_MODIFIER(active)
        else PARSE_MODIFIER(urgent)
        else PARSE_MODIFIER(local)
        else PARSE_MODIFIER(tiled)
        else PARSE_MODIFIER(monocle)
        else PARSE_MODIFIER(user_tiled)
        else PARSE_MODIFIER(user_monocle)
        else return false;
    }
    return true;
}

bool parse_node_modifiers(char *desc, node_select_t *sel)
{
    if (desc == NULL || sel == NULL) return false;
    char *tok;
    while ((tok = strrchr(desc, CAT_CHR)) != NULL) {
        *tok++ = '\0';
        if (strcmp("tiled", tok) == 0) sel->tiled = OPTION_TRUE;
        else if (strcmp("!tiled", tok) == 0) sel->tiled = OPTION_FALSE;
        else PARSE_MODIFIER(automatic)
        else PARSE_MODIFIER(focused)
        else PARSE_MODIFIER(active)
        else PARSE_MODIFIER(local)
        else PARSE_MODIFIER(leaf)
        else PARSE_MODIFIER(window)
        else PARSE_MODIFIER(pseudo_tiled)
        else PARSE_MODIFIER(floating)
        else PARSE_MODIFIER(fullscreen)
        else PARSE_MODIFIER(hidden)
        else PARSE_MODIFIER(sticky)
        else PARSE_MODIFIER(private)
        else PARSE_MODIFIER(locked)
        else PARSE_MODIFIER(marked)
        else PARSE_MODIFIER(urgent)
        else PARSE_MODIFIER(same_class)
        else PARSE_MODIFIER(descendant_of)
        else PARSE_MODIFIER(ancestor_of)
        else PARSE_MODIFIER(below)
        else PARSE_MODIFIER(normal)
        else PARSE_MODIFIER(above)
        else PARSE_MODIFIER(horizontal)
        else PARSE_MODIFIER(vertical)
        else return false;
    }
    return true;
}
