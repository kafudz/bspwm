/* Strict number parsing: the whole token must be a number in range. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/parse.h"

/* parse.c uses this helper; the test does not need the rest of helpers.c. */
char *copy_string(char *str, size_t len)
{
	char *cpy = calloc(1, len + 1);
	if (cpy != NULL && str != NULL) {
		memcpy(cpy, str, len);
	}
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

	double d;
	check("parse_double accepts an in-range value",
	      parse_double("0.5", 0.0, 1.0, &d) && d == 0.5);
	check("parse_double rejects trailing junk", !parse_double("0.5x", 0.0, 1.0, &d));
	check("parse_double rejects NaN", !parse_double("nan", 0.0, 1.0, &d));
	check("parse_double rejects infinity", !parse_double("inf", 0.0, 1.0, &d));
	check("parse_double rejects overflow", !parse_double("1e9999", 0.0, 1.0, &d));
	check("parse_double enforces the minimum", !parse_double("-0.1", 0.0, 1.0, &d));
	check("parse_double enforces the maximum", !parse_double("1.1", 0.0, 1.0, &d));

	int deg;
	check("parse_degree normalises 450 to 90", parse_degree("450", &deg) && deg == 90);
	check("parse_degree rejects trailing junk", !parse_degree("90x", &deg));
	check("parse_degree rejects overflow",
	      !parse_degree("99999999999999999999", &deg));
	check("parse_degree rejects a non-right angle", !parse_degree("45", &deg));

	uint32_t id;
	check("parse_id accepts a hexadecimal id", parse_id("0x2a", &id) && id == 42);
	check("parse_id rejects an empty string", !parse_id("", &id));
	check("parse_id rejects trailing junk", !parse_id("42x", &id));
	check("parse_id rejects overflow", !parse_id("0x1ffffffff", &id));

	uint16_t idx;
	check("parse_index accepts ^N", parse_index("^7", &idx) && idx == 7);
	check("parse_index rejects a bare ^", !parse_index("^", &idx));
	check("parse_index rejects a negative index", !parse_index("^-1", &idx));

	bspwm_rect_t r;
	check("parse_rectangle accepts a valid rectangle",
	      parse_rectangle("640x480+10+20", &r) && r.width == 640 &&
	      r.height == 480 && r.x == 10 && r.y == 20);
	check("parse_rectangle rejects an empty width", !parse_rectangle("x1+0+0", &r));
	check("parse_rectangle rejects an empty height", !parse_rectangle("1x+0+0", &r));
	check("parse_rectangle rejects an empty x", !parse_rectangle("1x1++0", &r));
	check("parse_rectangle rejects an empty y", !parse_rectangle("1x1+0+", &r));

	return failures != 0;
}
