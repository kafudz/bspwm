/* Minimal X11 client used by run_headless to exercise EWMH struts.
 *
 * Usage: test_strut_window L R T B LSY LEY RSY REY TSX TEX BSX BEX
 *
 * Commands are read from stdin, one per line:
 *   set, delete, map, unmap, geom X Y W H, quit
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <xcb/xcb.h>

static xcb_connection_t *dpy;
static xcb_window_t win;
static xcb_atom_t strut_atom, strut_partial_atom;
static uint32_t values[12];

static void install_strut(void)
{
	xcb_change_property(dpy, XCB_PROP_MODE_REPLACE, win, strut_partial_atom, XCB_ATOM_CARDINAL, 32, 12, values);
	xcb_change_property(dpy, XCB_PROP_MODE_REPLACE, win, strut_atom, XCB_ATOM_CARDINAL, 32, 4, values);
	xcb_flush(dpy);
}

static xcb_atom_t atom(const char *name)
{
	xcb_intern_atom_reply_t *r = xcb_intern_atom_reply(dpy, xcb_intern_atom(dpy, 0, strlen(name), name), NULL);
	xcb_atom_t a = r ? r->atom : XCB_ATOM_NONE;
	free(r);
	return a;
}

int main(int argc, char **argv)
{
	if (argc != 13) {
		fprintf(stderr, "usage: %s L R T B LSY LEY RSY REY TSX TEX BSX BEX\n", argv[0]);
		return 2;
	}
	for (int i = 0; i < 12; i++) {
		values[i] = (uint32_t) strtoul(argv[i + 1], NULL, 0);
	}

	dpy = xcb_connect(NULL, NULL);
	if (xcb_connection_has_error(dpy)) {
		fprintf(stderr, "Can't connect to X.\n");
		return EXIT_FAILURE;
	}
	xcb_screen_t *screen = xcb_setup_roots_iterator(xcb_get_setup(dpy)).data;
	if (!screen) {
		xcb_disconnect(dpy);
		return EXIT_FAILURE;
	}
	strut_atom = atom("_NET_WM_STRUT");
	strut_partial_atom = atom("_NET_WM_STRUT_PARTIAL");
	if (strut_atom == XCB_ATOM_NONE || strut_partial_atom == XCB_ATOM_NONE) {
		xcb_disconnect(dpy);
		return EXIT_FAILURE;
	}

	win = xcb_generate_id(dpy);
	xcb_create_window(dpy, XCB_COPY_FROM_PARENT, win, screen->root, 0, 0, 100, 100, 0,
	                  XCB_WINDOW_CLASS_INPUT_OUTPUT, XCB_COPY_FROM_PARENT, 0, NULL);
	install_strut();
	xcb_map_window(dpy, win);
	xcb_flush(dpy);
	puts("ready");
	fflush(stdout);

	char line[256];
	while (fgets(line, sizeof(line), stdin)) {
		if (!strncmp(line, "set", 3)) {
			install_strut();
		} else if (!strncmp(line, "delete", 6)) {
			xcb_delete_property(dpy, win, strut_partial_atom);
			xcb_delete_property(dpy, win, strut_atom);
			xcb_flush(dpy);
		} else if (!strncmp(line, "map", 3)) {
			xcb_map_window(dpy, win);
			xcb_flush(dpy);
		} else if (!strncmp(line, "unmap", 5)) {
			xcb_unmap_window(dpy, win);
			xcb_flush(dpy);
		} else if (!strncmp(line, "quit", 4)) {
			break;
		}
		puts("ok");
		fflush(stdout);
	}
	xcb_destroy_window(dpy, win);
	xcb_disconnect(dpy);
	return EXIT_SUCCESS;
}
