/* Copyright (c) 2012, Bastien Dejean
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include "backend_x11.h"
#include "bspwm.h"
#include "monitor.h"
#include "settings.h"
#include "tree.h"
#include "ewmh.h"

/* ewmh is defined in backend_x11.c */

void ewmh_init(void)
{
	backend_ewmh_init();
}

void ewmh_update_active_window(void)
{
	ensure_focused_monitor();
	xcb_window_t win = ((mon == NULL || mon->desk == NULL || mon->desk->focus == NULL ||
	                      mon->desk->focus->client == NULL) ? XCB_NONE : mon->desk->focus->id);
	xcb_ewmh_set_active_window(ewmh, default_screen, win);
}

void ewmh_update_number_of_desktops(void)
{
	uint32_t desktops_count = 0;

	for (monitor_t *m = mon_head; m != NULL; m = m->next) {
		for (desktop_t *d = m->desk_head; d != NULL; d = d->next) {
			desktops_count++;
		}
	}

	xcb_ewmh_set_number_of_desktops(ewmh, default_screen, desktops_count);
}

uint32_t ewmh_get_desktop_index(desktop_t *d)
{
	uint32_t i = 0;
	for (monitor_t *m = mon_head; m != NULL; m = m->next) {
		for (desktop_t *cd = m->desk_head; cd != NULL; cd = cd->next, i++) {
			if (d == cd) {
				return i;
			}
		}
	}
	return 0;
}

bool ewmh_locate_desktop(uint32_t i, coordinates_t *loc)
{
	for (monitor_t *m = mon_head; m != NULL; m = m->next) {
		for (desktop_t *d = m->desk_head; d != NULL; d = d->next, i--) {
			if (i == 0) {
				loc->monitor = m;
				loc->desktop = d;
				loc->node = NULL;
				return true;
			}
		}
	}
	return false;
}

void ewmh_update_current_desktop(void)
{
	if (mon == NULL || mon->desk == NULL) {
		return;
	}
	uint32_t i = ewmh_get_desktop_index(mon->desk);
	xcb_ewmh_set_current_desktop(ewmh, default_screen, i);
}

void ewmh_set_wm_desktop(node_t *n, desktop_t *d)
{
	uint32_t i = ewmh_get_desktop_index(d);
	for (node_t *f = first_extrema(n); f != NULL; f = next_leaf(f, n)) {
		if (f->client == NULL) {
			continue;
		}
		xcb_ewmh_set_wm_desktop(ewmh, f->id, i);
	}
}

void ewmh_update_wm_desktops(void)
{
	for (monitor_t *m = mon_head; m != NULL; m = m->next) {
		for (desktop_t *d = m->desk_head; d != NULL; d = d->next) {
			uint32_t i = ewmh_get_desktop_index(d);
			for (node_t *n = first_extrema(d->root); n != NULL; n = next_leaf(n, d->root)) {
				if (n->client == NULL) {
					continue;
				}
				xcb_ewmh_set_wm_desktop(ewmh, n->id, i);
			}
		}
	}
}

void ewmh_update_desktop_names(void)
{
	char *names = NULL;
	size_t names_size = 0;
	size_t names_capacity = MAXLEN;
	size_t current_pos = 0;

	names = malloc(names_capacity);
	if (names == NULL) {
		xcb_ewmh_set_desktop_names(ewmh, default_screen, 0, NULL);
		return;
	}

	for (monitor_t *m = mon_head; m != NULL; m = m->next) {
		for (desktop_t *d = m->desk_head; d != NULL; d = d->next) {
			size_t name_len = strlen(d->name);

			/* +1 for null terminator */
			while (current_pos + name_len + 1 > names_capacity) {
				size_t new_capacity = names_capacity * 2;
				if (new_capacity > INT_MAX) {
					free(names);
					xcb_ewmh_set_desktop_names(ewmh, default_screen, 0, NULL);
					return;
				}
				char *new_names = realloc(names, new_capacity);
				if (new_names == NULL) {
					free(names);
					names = NULL;
					xcb_ewmh_set_desktop_names(ewmh, default_screen, 0, NULL);
					return;
				}
				names = new_names;
				names_capacity = new_capacity;
			}

			memcpy(names + current_pos, d->name, name_len);
			current_pos += name_len;
			names[current_pos++] = '\0';
			names_size = current_pos;
		}
	}

	if (names_size > 0) {
		names_size--; /* Don't count the final null */
		xcb_ewmh_set_desktop_names(ewmh, default_screen, names_size, names);
	} else {
		xcb_ewmh_set_desktop_names(ewmh, default_screen, 0, NULL);
	}

	free(names);
}

void ewmh_update_desktop_viewport(void)
{
	uint32_t desktops_count = 0;
	for (monitor_t *m = mon_head; m != NULL; m = m->next) {
		for (desktop_t *d = m->desk_head; d != NULL; d = d->next) {
			desktops_count++;
		}
	}
	if (desktops_count == 0) {
		xcb_ewmh_set_desktop_viewport(ewmh, default_screen, 0, NULL);
		return;
	}
	xcb_ewmh_coordinates_t *coords = calloc(desktops_count, sizeof(xcb_ewmh_coordinates_t));
	if (coords == NULL) {
		return;
	}
	uint16_t desktop = 0;
	for (monitor_t *m = mon_head; m != NULL; m = m->next) {
		for (desktop_t *d = m->desk_head; d != NULL; d = d->next) {
			coords[desktop++] = (xcb_ewmh_coordinates_t){m->rectangle.x, m->rectangle.y};
		}
	}
	xcb_ewmh_set_desktop_viewport(ewmh, default_screen, desktop, coords);
	free(coords);
}

typedef struct strut_owner_t strut_owner_t;
struct strut_owner_t {
	bspwm_wid_t win;
	xcb_ewmh_wm_strut_partial_t strut;
	strut_owner_t *prev;
	strut_owner_t *next;
};

static strut_owner_t *strut_owners = NULL;

static strut_owner_t *find_strut_owner(bspwm_wid_t win)
{
	for (strut_owner_t *o = strut_owners; o != NULL; o = o->next) {
		if (o->win == win) {
			return o;
		}
	}
	return NULL;
}

static void free_strut_owner(strut_owner_t *o)
{
	if (!o) {
		return;
	}
	if (o->prev) {
		o->prev->next = o->next;
	} else {
		strut_owners = o->next;
	}
	if (o->next) {
		o->next->prev = o->prev;
	}
	free(o);
}

void ewmh_struts_clear(void)
{
	while (strut_owners) {
		free_strut_owner(strut_owners);
	}
}

/* All strut values are CARDINALs on the wire.  Never cast them to int16_t:
 * a value >= 32768 would wrap negative.  Clamp to the screen dimension so a
 * malformed claim cannot reserve an unbounded amount of padding. */
static int64_t strut_value(uint32_t raw, int limit)
{
	if (limit < 0) {
		limit = 0;
	}
	if (raw > (uint32_t) limit) {
		return limit;
	}
	return (int64_t) raw;
}

/* An extent larger than the screen is malformed.  Ignoring it is safer than
 * clamping it to the screen edge, which would reserve the whole screen. */
static bool valid_extent(uint32_t raw, int limit)
{
	return limit > 0 && raw <= (uint32_t) limit;
}

/* A 0/0 range means "the whole edge" (the convention used by
 * _NET_WM_STRUT).  Any other reversed range is malformed and claims nothing. */
static bool strut_range_overlaps(uint32_t raw_start, uint32_t raw_end, int limit,
                                  int64_t rect_start, int64_t rect_end)
{
	int64_t start = strut_value(raw_start, limit);
	int64_t end = strut_value(raw_end, limit);
	if (start == 0 && end == 0) {
		start = 0;
		end = limit;
	} else if (start >= end) {
		return false;
	}
	return start < rect_end && end > rect_start;
}

static bool read_strut(bspwm_wid_t win, xcb_ewmh_wm_strut_partial_t *s)
{
	memset(s, 0, sizeof(*s));

	/* Issue both requests before waiting so the fallback costs no extra
	 * round trip for the common (partial property present) case. */
	xcb_get_property_cookie_t partial_cookie = xcb_ewmh_get_wm_strut_partial(ewmh, win);
	xcb_get_property_cookie_t full_cookie = xcb_get_property(dpy, 0, win, ewmh->_NET_WM_STRUT,
	                                                         XCB_ATOM_CARDINAL, 0, 4);
	if (xcb_ewmh_get_wm_strut_partial_reply(ewmh, partial_cookie, s, NULL) == 1) {
		xcb_generic_error_t *err = NULL;
		free(xcb_get_property_reply(dpy, full_cookie, &err));
		free(err);
		return true;
	}

	/* Fall back to the older, range-less _NET_WM_STRUT property. */
	xcb_get_property_reply_t *reply = xcb_get_property_reply(dpy, full_cookie, NULL);
	if (reply == NULL || reply->type != XCB_ATOM_CARDINAL || reply->format != 32 ||
	    xcb_get_property_value_length(reply) < 4 * (int) sizeof(uint32_t)) {
		free(reply);
		return false;
	}
	uint32_t *v = xcb_get_property_value(reply);
	s->left = v[0];
	s->right = v[1];
	s->top = v[2];
	s->bottom = v[3];
	free(reply);
	return true;
}

bool ewmh_struts_update(void)
{
	bool changed = false;

	for (monitor_t *m = mon_head; m != NULL; m = m->next) {
		padding_t claim = {0, 0, 0, 0};
		bspwm_rect_t r = m->rectangle;
		int64_t rx2 = (int64_t) r.x + r.width;
		int64_t ry2 = (int64_t) r.y + r.height;

		for (strut_owner_t *o = strut_owners; o != NULL; o = o->next) {
			xcb_ewmh_wm_strut_partial_t *s = &o->strut;
			bool active = !ignore_ewmh_struts;

			if (active && valid_extent(s->left, screen_width) &&
			    strut_range_overlaps(s->left_start_y, s->left_end_y, screen_height, r.y, ry2)) {
				int64_t extent = strut_value(s->left, screen_width);
				int64_t v = extent - r.x;
				if (v > 0) {
					claim.left = MAX(claim.left, (int) MIN(v, (int64_t) r.width));
				}
			}
			if (active && valid_extent(s->right, screen_width) &&
			    strut_range_overlaps(s->right_start_y, s->right_end_y, screen_height, r.y, ry2)) {
				int64_t extent = strut_value(s->right, screen_width);
				int64_t edge = (int64_t) screen_width - rx2;
				int64_t v = extent - edge;
				if (v > 0) {
					claim.right = MAX(claim.right, (int) MIN(v, (int64_t) r.width));
				}
			}
			if (active && valid_extent(s->top, screen_height) &&
			    strut_range_overlaps(s->top_start_x, s->top_end_x, screen_width, r.x, rx2)) {
				int64_t extent = strut_value(s->top, screen_height);
				int64_t v = extent - r.y;
				if (v > 0) {
					claim.top = MAX(claim.top, (int) MIN(v, (int64_t) r.height));
				}
			}
			if (active && valid_extent(s->bottom, screen_height) &&
			    strut_range_overlaps(s->bottom_start_x, s->bottom_end_x, screen_width, r.x, rx2)) {
				int64_t extent = strut_value(s->bottom, screen_height);
				int64_t edge = (int64_t) screen_height - ry2;
				int64_t v = extent - edge;
				if (v > 0) {
					claim.bottom = MAX(claim.bottom, (int) MIN(v, (int64_t) r.height));
				}
			}
		}

		if (claim.top != m->strut_padding.top || claim.right != m->strut_padding.right ||
		    claim.bottom != m->strut_padding.bottom || claim.left != m->strut_padding.left) {
			changed = true;
		}
		m->strut_padding = claim;
		monitor_refresh_padding(m);
	}
	return changed;
}

void ewmh_struts_recompute_all(void)
{
	if (!ewmh_struts_update()) {
		return;
	}
	for (monitor_t *m = mon_head; m != NULL; m = m->next) {
		for (desktop_t *d = m->desk_head; d != NULL; d = d->next) {
			arrange(m, d);
		}
	}
}

bool ewmh_strut_is_owner(bspwm_wid_t win)
{
	return find_strut_owner(win) != NULL;
}

void ewmh_struts_remove_owner(bspwm_wid_t win)
{
	strut_owner_t *o = find_strut_owner(win);
	if (!o) {
		return;
	}
	free_strut_owner(o);
	ewmh_struts_recompute_all();
}

bool ewmh_handle_struts(bspwm_wid_t win)
{
	xcb_ewmh_wm_strut_partial_t s;
	if (!read_strut(win, &s)) {
		strut_owner_t *o = find_strut_owner(win);
		if (!o) {
			return false;
		}
		free_strut_owner(o);
		return ewmh_struts_update();
	}

	strut_owner_t *o = find_strut_owner(win);
	if (!o) {
		o = calloc(1, sizeof(*o));
		if (!o) {
			return false;
		}
		o->win = win;
		o->next = strut_owners;
		if (strut_owners) {
			strut_owners->prev = o;
		}
		strut_owners = o;
	}
	o->strut = s;
	return ewmh_struts_update();
}

void ewmh_update_client_list(bool stacking)
{
	if (clients_count == 0) {
		xcb_ewmh_set_client_list(ewmh, default_screen, 0, NULL);
		xcb_ewmh_set_client_list_stacking(ewmh, default_screen, 0, NULL);
		return;
	}

	xcb_window_t *wins = calloc(clients_count, sizeof(xcb_window_t));
	if (wins == NULL) {
		return;
	}

	unsigned int i = 0;

	if (stacking) {
		for (stacking_list_t *s = stack_head; s != NULL && i < clients_count; s = s->next) {
			wins[i++] = s->node->id;
		}
		xcb_ewmh_set_client_list_stacking(ewmh, default_screen, i, wins);
	} else {
		for (monitor_t *m = mon_head; m != NULL && i < clients_count; m = m->next) {
			for (desktop_t *d = m->desk_head; d != NULL && i < clients_count; d = d->next) {
				for (node_t *n = first_extrema(d->root); n != NULL && i < clients_count; n = next_leaf(n, d->root)) {
					if (n->client == NULL) {
						continue;
					}
					wins[i++] = n->id;
				}
			}
		}
		xcb_ewmh_set_client_list(ewmh, default_screen, i, wins);
	}

	free(wins);
}

void ewmh_update_client_lists(void)
{
	if (clients_count == 0) {
		xcb_ewmh_set_client_list(ewmh, default_screen, 0, NULL);
		xcb_ewmh_set_client_list_stacking(ewmh, default_screen, 0, NULL);
		return;
	}

	xcb_window_t *wins = calloc(clients_count, sizeof(xcb_window_t));
	if (wins == NULL) {
		return;
	}

	unsigned int i = 0;
	for (monitor_t *m = mon_head; m != NULL && i < clients_count; m = m->next) {
		for (desktop_t *d = m->desk_head; d != NULL && i < clients_count; d = d->next) {
			for (node_t *n = first_extrema(d->root); n != NULL && i < clients_count; n = next_leaf(n, d->root)) {
				if (n->client == NULL) {
					continue;
				}
				wins[i++] = n->id;
			}
		}
	}
	xcb_ewmh_set_client_list(ewmh, default_screen, i, wins);

	i = 0;
	for (stacking_list_t *s = stack_head; s != NULL && i < clients_count; s = s->next) {
		wins[i++] = s->node->id;
	}
	xcb_ewmh_set_client_list_stacking(ewmh, default_screen, i, wins);

	free(wins);
}

void ewmh_wm_state_update(node_t *n)
{
	client_t *c = n->client;
	size_t count = 0;
	uint32_t values[12];
#define HANDLE_WM_STATE(s)  \
	if (WM_FLAG_##s & c->wm_flags) { \
		values[count++] = ewmh->_NET_WM_STATE_##s; \
	}
	HANDLE_WM_STATE(MODAL)
	HANDLE_WM_STATE(STICKY)
	HANDLE_WM_STATE(MAXIMIZED_VERT)
	HANDLE_WM_STATE(MAXIMIZED_HORZ)
	HANDLE_WM_STATE(SHADED)
	HANDLE_WM_STATE(SKIP_TASKBAR)
	HANDLE_WM_STATE(SKIP_PAGER)
	HANDLE_WM_STATE(HIDDEN)
	HANDLE_WM_STATE(FULLSCREEN)
	HANDLE_WM_STATE(ABOVE)
	HANDLE_WM_STATE(BELOW)
	HANDLE_WM_STATE(DEMANDS_ATTENTION)
#undef HANDLE_WM_STATE
	xcb_ewmh_set_wm_state(ewmh, n->id, count, values);
}

void ewmh_set_supporting(xcb_window_t win)
{
	pid_t wm_pid = getpid();
	xcb_ewmh_set_supporting_wm_check(ewmh, root, win);
	xcb_ewmh_set_supporting_wm_check(ewmh, win, win);
	xcb_ewmh_set_wm_name(ewmh, win, strlen(WM_NAME), WM_NAME);
	xcb_ewmh_set_wm_pid(ewmh, win, wm_pid);
}
