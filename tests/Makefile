# C23 is spelled -std=c2x before GCC 14; use whichever this compiler accepts.
cc-option = $(shell printf 'int main(void){return 0;}\n' | $(CC) -Werror $(1) -x c -c -o /dev/null - >/dev/null 2>&1 && echo $(1))
ifndef STD
STD := $(firstword $(foreach s,c23 c2x,$(if $(call cc-option,-std=$(s)),$(s))) c2x)
endif

CFLAGS  += -std=c99 -D_POSIX_C_SOURCE=200809L -pedantic -Wall -Wextra

all: test_window test_rule_match test_magnet test_jsmn test_geometry test_parse

test_window: test_window.c
	$(CC) $(CFLAGS) -o $@ $< -lxcb -lxcb-icccm

test_rule_match: test_rule_match.c ../src/rule_match.c ../src/rule_match.h
	$(CC) $(CFLAGS) -std=$(STD) -o $@ test_rule_match.c ../src/rule_match.c

test_magnet: test_magnet.c ../src/magnet.c ../src/magnet.h
	$(CC) $(CFLAGS) -std=$(STD) -o $@ test_magnet.c ../src/magnet.c

test_jsmn: test_jsmn.c ../src/jsmn.c ../src/jsmn.h
	$(CC) $(CFLAGS) -std=$(STD) -DJSMN_STRICT -o $@ test_jsmn.c ../src/jsmn.c

test_geometry: test_geometry.c ../src/geometry.c ../src/geometry.h
	$(CC) $(CFLAGS) -std=$(STD) -o $@ test_geometry.c ../src/geometry.c

test_parse: test_parse.c ../src/parse.c ../src/parse.h
	$(CC) $(CFLAGS) -std=$(STD) -o $@ test_parse.c ../src/parse.c

clean:
	$(RM) test_window test_rule_match test_magnet test_jsmn test_geometry test_parse *.o

.PHONY: all clean
