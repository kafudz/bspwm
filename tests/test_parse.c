/* Strict parser regression tests. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "../src/parse.h"

char *copy_string(char *str, size_t len)
{
	char *cpy = calloc(1, len + 1);
	if (cpy != NULL && str != NULL) memcpy(cpy, str, len);
	return cpy;
}

static int failures;

static void check(const char *desc, int ok)
{
	printf("  %s: %s\n", ok ? "PASS" : "FAIL", desc);
	failures += !ok;
}

int main(void)
{
	int i;
	check("parse_int accepts an in-range value", parse_int("42", 0, 100, &i) && i == 42);
	check("parse_int rejects trailing junk", !parse_int("42x", 0, 100, &i));
	check("parse_int rejects an empty string", !parse_int("", 0, 100, &i));
	check("parse_int rejects a value below the minimum", !parse_int("-1", 0, 100, &i));
	check("parse_int rejects a value above the maximum", !parse_int("101", 0, 100, &i));
	check("parse_int rejects overflow", !parse_int("99999999999999999999", 0, 100, &i));
	check("parse_int rejects narrowing outside int range",
	      !parse_int("2147483648", LONG_MIN, LONG_MAX, &i));
	check("parse_int rejects an inverted range", !parse_int("1", 2, 0, &i));
	check("parse_int rejects null input", !parse_int(NULL, 0, 1, &i));
	check("parse_int rejects null output", !parse_int("1", 0, 2, NULL));

	double d;
	check("parse_double accepts an in-range value", parse_double("0.5", 0.0, 1.0, &d) && d == 0.5);
	check("parse_double rejects trailing junk", !parse_double("0.5x", 0.0, 1.0, &d));
	check("parse_double rejects NaN", !parse_double("nan", 0.0, 1.0, &d));
	check("parse_double rejects infinity", !parse_double("inf", 0.0, 1.0, &d));
	check("parse_double rejects overflow", !parse_double("1e9999", 0.0, 1.0, &d));
	check("parse_double rejects inverted range", !parse_double("0.5", 1.0, 0.0, &d));
	check("parse_double rejects null input", !parse_double(NULL, 0.0, 1.0, &d));
	check("parse_double rejects null output", !parse_double("0.5", 0.0, 1.0, NULL));

	int deg;
	check("parse_degree normalises 450 to 90", parse_degree("450", &deg) && deg == 90);
	check("parse_degree rejects trailing junk", !parse_degree("90x", &deg));
	check("parse_degree rejects overflow", !parse_degree("99999999999999999999", &deg));
	check("parse_degree rejects a non-right angle", !parse_degree("45", &deg));
	check("parse_degree rejects null arguments", !parse_degree(NULL, &deg) && !parse_degree("90", NULL));

	uint32_t id;
	check("parse_id accepts a hexadecimal id", parse_id("0x2a", &id) && id == 42);
	check("parse_id rejects an empty string", !parse_id("", &id));
	check("parse_id rejects trailing junk", !parse_id("42x", &id));
	check("parse_id rejects overflow", !parse_id("0x1ffffffff", &id));
	check("parse_id rejects null arguments", !parse_id(NULL, &id) && !parse_id("42", NULL));

	uint16_t idx;
	check("parse_index accepts ^N", parse_index("^7", &idx) && idx == 7);
	check("parse_index rejects a bare ^", !parse_index("^", &idx));
	check("parse_index rejects a negative index", !parse_index("^-1", &idx));
	check("parse_index rejects null arguments", !parse_index(NULL, &idx) && !parse_index("^1", NULL));

	bspwm_rect_t r;
	check("parse_rectangle accepts a valid rectangle",
	      parse_rectangle("640x480+10+20", &r) && r.width == 640 && r.height == 480 && r.x == 10 && r.y == 20);
	check("parse_rectangle rejects empty dimensions",
	      !parse_rectangle("x1+0+0", &r) && !parse_rectangle("1x+0+0", &r));
	check("parse_rectangle rejects missing coordinates",
	      !parse_rectangle("1x1++0", &r) && !parse_rectangle("1x1+0+", &r));
	check("parse_rectangle rejects numeric overflow", !parse_rectangle("999999999999999999999x1+0+0", &r));
	check("parse_rectangle rejects null arguments", !parse_rectangle(NULL, &r) && !parse_rectangle("1x1+0+0", NULL));

	state_transition_t transition;
	check("state transition accepts a list", parse_state_transition("enter,exit", &transition));
	check("state transition rejects empty list entries", !parse_state_transition("enter,,exit", &transition));
	check("state transition rejects trailing comma", !parse_state_transition("enter,", &transition));
	check("state transition rejects null arguments", !parse_state_transition(NULL, &transition) && !parse_state_transition("all", NULL));

	bool value;
	char declaration[] = "enabled=true";
	char malformed[] = "enabled=true=off";
	char *key;
	alter_state_t state;
	check("boolean declaration parses a setting", parse_bool_declaration(declaration, &key, &value, &state) && strcmp(key, "enabled") == 0 && value && state == ALTER_SET);
	check("boolean declaration rejects extra values", !parse_bool_declaration(malformed, &key, &value, &state));
	check("boolean declaration rejects null arguments", !parse_bool_declaration(NULL, &key, &value, &state));

	return failures != 0;
}
