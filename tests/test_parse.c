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

	return failures != 0;
}
