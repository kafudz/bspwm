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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>
#include <errno.h>
#include <poll.h>
#include <time.h>
#include "bspwm.h"
#include "common.h"

/* Largest gap between chunks read from a FIFO before giving up, and the
 * total time such a read may occupy the event loop. */
#define READ_STRING_TIMEOUT_MS   5000
#define READ_STRING_DEADLINE_MS  30000

static long long monotonic_ms(void)
{
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) == -1) {
        return -1;
    }
    return (long long) ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

/* Mark as cold - rarely executed, improves instruction cache (nomt-style) */
__attribute__((cold))
void warn(char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
}

/* Build the control-socket path for the given display coordinates. Prefers
 * $XDG_RUNTIME_DIR (per-user, 0700, not swept by the /tmp timer) and falls
 * back to /tmp only when it is unset or empty. bspwm and bspc both resolve
 * the path through here so they always agree. Returns the snprintf() result:
 * negative on encoding error, or >= buflen when the path was truncated —
 * callers must treat either as "path too long". Two literal format strings
 * keep -Wformat=2 (-Wformat-nonliteral) quiet. */
int make_socket_path(char *buf, size_t buflen, const char *host, int dn, int sn)
{
	const char *runtime = getenv("XDG_RUNTIME_DIR");
	if (runtime != NULL && runtime[0] != '\0') {
		return snprintf(buf, buflen, SOCKET_PATH_TPL_XDG, runtime, host, dn, sn);
	}
	return snprintf(buf, buflen, SOCKET_PATH_TPL, host, dn, sn);
}

/* State-file path, same $XDG_RUNTIME_DIR-preferred rule as the socket. Kept
 * out of world-writable /tmp so another uid cannot plant, symlink, or FIFO
 * the file bspwm re-reads across a restart. */
int make_state_path(char *buf, size_t buflen, const char *host, int dn, int sn)
{
	const char *runtime = getenv("XDG_RUNTIME_DIR");
	if (runtime != NULL && runtime[0] != '\0') {
		return snprintf(buf, buflen, STATE_PATH_TPL_XDG, runtime, host, dn, sn);
	}
	return snprintf(buf, buflen, STATE_PATH_TPL, host, dn, sn);
}

__attribute__((cold, noreturn))
void err(char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	exit(EXIT_FAILURE);
}

char *read_string(const char *file_path, size_t *tlen)
{
    if (file_path == NULL) {
        return NULL;
    }

    /* O_NONBLOCK keeps open() from blocking forever on a FIFO with no
     * writer. Regular files ignore it. */
    int fd = open(file_path, O_RDONLY | O_CLOEXEC | O_NONBLOCK);
    if (fd == -1) {
        perror("Read file: open");
        return NULL;
    }

    struct stat st;
    if (fstat(fd, &st) == -1) {
        perror("Read file: fstat");
        close(fd);
        return NULL;
    }

    if ((!S_ISREG(st.st_mode) && !S_ISFIFO(st.st_mode)) || st.st_uid != getuid()) {
        warn("Refusing to read '%s': not a regular file or FIFO owned by uid %u.\n",
             file_path, (unsigned) getuid());
        close(fd);
        return NULL;
    }

    if (st.st_size > MAX_STRING_SIZE) {
        warn("File too large: %lld bytes (max: %d)\n", (long long)st.st_size, MAX_STRING_SIZE);
        close(fd);
        return NULL;
    }

    char buf[BUFSIZ], *content = NULL;
    size_t len = sizeof(buf);

    if ((content = calloc(len, sizeof(char))) == NULL) {
        perror("Read file: calloc");
        close(fd);
        return NULL;
    }

    *tlen = 0;

    /* A FIFO writer that trickles data could otherwise extend the read
     * indefinitely; the absolute deadline bounds the whole operation. */
    long long deadline = monotonic_ms();
    if (deadline >= 0) {
        deadline += READ_STRING_DEADLINE_MS;
    }

    while (true) {
        /* A FIFO can stay open with no data forever; waiting with a bound
         * keeps `bspc wm -l <fifo>` from freezing the whole WM event loop.
         * Regular files are always ready and need no poll. */
        if (S_ISFIFO(st.st_mode)) {
            int timeout = READ_STRING_TIMEOUT_MS;
            long long now = monotonic_ms();
            if (deadline >= 0 && now >= 0) {
                long long remaining = deadline - now;
                if (remaining <= 0) {
                    warn("Timed out reading '%s'.\n", file_path);
                    free(content);
                    close(fd);
                    return NULL;
                }
                if (remaining < timeout) {
                    timeout = (int) remaining;
                }
            }
            struct pollfd pfd = {.fd = fd, .events = POLLIN};
            int pr;
            do {
                pr = poll(&pfd, 1, timeout);
            } while (pr == -1 && errno == EINTR);
            if (pr == 0) {
                warn("Timed out reading '%s'.\n", file_path);
                free(content);
                close(fd);
                return NULL;
            }
            if (pr == -1) {
                perror("Read file: poll");
                free(content);
                close(fd);
                return NULL;
            }
        }

        ssize_t nb = read(fd, buf, sizeof(buf));
        if (nb < 0) {
            if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK) {
                continue;
            }
            perror("Read file: read");
            free(content);
            close(fd);
            return NULL;
        }
        if (nb == 0) {
            break;
        }

        size_t need = *tlen + (size_t) nb + 1; /* + terminating NUL */
        if (need > MAX_STRING_SIZE + 1) {
            warn("Content too large while reading\n");
            free(content);
            close(fd);
            return NULL;
        }
        if (need > len) {
            size_t new_len = len;
            while (new_len < need) {
                new_len *= 2;
            }
            if (new_len > MAX_STRING_SIZE + 1) {
                new_len = MAX_STRING_SIZE + 1;
            }
            char *rcontent = realloc(content, new_len * sizeof(char));
            if (rcontent == NULL) {
                perror("Read file: realloc");
                free(content);
                close(fd);
                return NULL;
            }
            content = rcontent;
            len = new_len;
        }
        memcpy(content + *tlen, buf, nb);
        *tlen += (size_t) nb;
    }

    close(fd);
    content[*tlen] = '\0';

    return content;
}


char *copy_string(char *str, size_t len)
{
	if (str == NULL) {
		return NULL;
	}
	char *cpy = calloc(1, len + 1);
	if (cpy == NULL) {
		perror("Copy string: calloc");
		return NULL;
	}
	memcpy(cpy, str, len);
	cpy[len] = '\0';
	return cpy;
}

char *mktempfifo(const char *template)
{
	int tempfd;
	char *runtime_dir = getenv(RUNTIME_DIR_ENV);
	if (runtime_dir == NULL) {
		runtime_dir = "/tmp";
	}

	size_t runtime_len = strlen(runtime_dir);
	size_t template_len = strlen(template);
	size_t path_len = runtime_len + 1 + template_len + 1;

	char *fifo_path;
	fifo_path = malloc(path_len);
	if (fifo_path == NULL) {
		return NULL;
	}

	memcpy(fifo_path, runtime_dir, runtime_len);
	fifo_path[runtime_len] = '/';
	memcpy(fifo_path + runtime_len + 1, template, template_len + 1);

	if ((tempfd = mkstemp(fifo_path)) == -1) {
		free(fifo_path);
		return NULL;
	}

	close(tempfd);
	unlink(fifo_path);

	if (mkfifo(fifo_path, 0600) == -1) {
		free(fifo_path);
		return NULL;
	}

	/* mkfifo() cannot use O_EXCL: another process could have won the name
	 * between the unlink and the call. Verify what we ended up with before
	 * handing the path to a subscriber. */
	struct stat st;
	if (lstat(fifo_path, &st) == -1 || !S_ISFIFO(st.st_mode) ||
	    st.st_uid != getuid()) {
		unlink(fifo_path);
		free(fifo_path);
		return NULL;
	}

	return fifo_path;
}

int asprintf(char **buf, const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	int size = vasprintf(buf, fmt, args);
	va_end(args);
	return size;
}

int vasprintf(char **buf, const char *fmt, va_list args)
{
	va_list tmp;
	va_copy(tmp, args);
	int size = vsnprintf(NULL, 0, fmt, tmp);
	va_end(tmp);

	if (size < 0) {
		return -1;
	}

	*buf = malloc(size + 1);

	if (*buf == NULL) {
		return -1;
	}

	size = vsprintf(*buf, fmt, args);
	return size;
}

bool is_hex_color(const char *color)
{
	if (color[0] != '#' || strlen(color) != 7) {
		return false;
	}
	for (int i = 1; i < 7; i++) {
		if (!isxdigit(color[i])) {
			return false;
		}
	}
	return true;
}

char *tokenize_with_escape(struct tokenize_state *state, const char *s, char sep)
{
	if (s != NULL) {
		// first call
		state->in_escape = false;
		state->pos = s;
		state->len = strlen(s) + 1;
	}

	char *outp = calloc(state->len, 1);
	char *ret = outp;
	if (!outp) return NULL;

	char cur;
	while (*state->pos) {
		--state->len;
		cur = *state->pos++;

		if (state->in_escape) {
			*outp++ = cur;
			state->in_escape = false;
			continue;
		}

		if (cur == '\\') {
			state->in_escape = !state->in_escape;
		} else if (cur == sep) {
			return ret;
		} else {
			*outp++ = cur;
		}
	}

	return ret;
}
