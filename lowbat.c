#define _GNU_SOURCE

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <wayland-client.h>

#include "config.h"
#include "wlr-layer-shell-unstable-v1-client-protocol.h"

#ifndef VERSION
#define VERSION "1.0"
#endif

struct glyph {
	char c;
	uint8_t r[7];
};

static void kbenter(void *d, struct wl_keyboard *k, uint32_t s, struct wl_surface *sf, struct wl_array *keys);
static void kbkey(void *d, struct wl_keyboard *k, uint32_t s, uint32_t t, uint32_t key, uint32_t st);
static void kbleave(void *d, struct wl_keyboard *k, uint32_t s, struct wl_surface *sf);
static void kbmap(void *d, struct wl_keyboard *k, uint32_t f, int32_t fd, uint32_t sz);
static void kbmodifiers(void *d, struct wl_keyboard *k, uint32_t s, uint32_t dep, uint32_t lat, uint32_t lck, uint32_t grp);
static void kbrepeatinfo(void *d, struct wl_keyboard *k, int32_t rate, int32_t delay);
static void lscfg(void *d, struct zwlr_layer_surface_v1 *l, uint32_t s, uint32_t w, uint32_t h);
static void lsclosed(void *d, struct zwlr_layer_surface_v1 *l);
static void ptraxis(void *d, struct wl_pointer *p, uint32_t t, uint32_t a, wl_fixed_t v);
static void ptrbtn(void *d, struct wl_pointer *p, uint32_t s, uint32_t t, uint32_t b, uint32_t st);
static void ptrenter(void *d, struct wl_pointer *p, uint32_t s, struct wl_surface *sf, wl_fixed_t sx, wl_fixed_t sy);
static void ptrframe(void *d, struct wl_pointer *p);
static void ptrleave(void *d, struct wl_pointer *p, uint32_t s, struct wl_surface *sf);
static void ptrmotion(void *d, struct wl_pointer *p, uint32_t t, wl_fixed_t sx, wl_fixed_t sy);
static void regglob(void *d, struct wl_registry *r, uint32_t n, const char *i, uint32_t v);
static void regremove(void *d, struct wl_registry *r, uint32_t n);
static void seatcaps(void *d, struct wl_seat *s, uint32_t c);
static void seatname(void *d, struct wl_seat *s, const char *name);

const struct wl_interface xdg_popup_interface;

static const struct glyph font[] = {
	{ '0', { 0x0E, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0E } },
	{ '1', { 0x04, 0x0C, 0x04, 0x04, 0x04, 0x04, 0x0E } },
	{ '2', { 0x0E, 0x11, 0x01, 0x02, 0x04, 0x08, 0x1F } },
	{ '3', { 0x1E, 0x01, 0x01, 0x0E, 0x01, 0x01, 0x1E } },
	{ '4', { 0x02, 0x06, 0x0A, 0x12, 0x1F, 0x02, 0x02 } },
	{ '5', { 0x1F, 0x10, 0x1E, 0x01, 0x01, 0x11, 0x0E } },
	{ '6', { 0x06, 0x08, 0x10, 0x1E, 0x11, 0x11, 0x0E } },
	{ '7', { 0x1F, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08 } },
	{ '8', { 0x0E, 0x11, 0x11, 0x0E, 0x11, 0x11, 0x0E } },
	{ '9', { 0x0E, 0x11, 0x11, 0x0F, 0x01, 0x02, 0x0C } },
	{ '%', { 0x19, 0x1A, 0x02, 0x04, 0x08, 0x0B, 0x13 } },
	{ 'A', { 0x0E, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11 } },
	{ 'B', { 0x1E, 0x11, 0x11, 0x1E, 0x11, 0x11, 0x1E } },
	{ 'C', { 0x0E, 0x11, 0x10, 0x10, 0x10, 0x11, 0x0E } },
	{ 'E', { 0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x1F } },
	{ 'G', { 0x0E, 0x11, 0x10, 0x17, 0x11, 0x11, 0x0F } },
	{ 'H', { 0x11, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11 } },
	{ 'I', { 0x0E, 0x04, 0x04, 0x04, 0x04, 0x04, 0x0E } },
	{ 'L', { 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1F } },
	{ 'N', { 0x11, 0x19, 0x15, 0x13, 0x11, 0x11, 0x11 } },
	{ 'O', { 0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E } },
	{ 'P', { 0x1E, 0x11, 0x11, 0x1E, 0x10, 0x10, 0x10 } },
	{ 'R', { 0x1E, 0x11, 0x11, 0x1E, 0x14, 0x12, 0x11 } },
	{ 'T', { 0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04 } },
	{ 'U', { 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E } },
	{ 'W', { 0x11, 0x11, 0x11, 0x15, 0x15, 0x1B, 0x11 } },
	{ 'Y', { 0x11, 0x11, 0x0A, 0x04, 0x04, 0x04, 0x04 } }
};

static const struct wl_pointer_listener ptrlistener = {
	.enter = ptrenter,
	.leave = ptrleave,
	.motion = ptrmotion,
	.button = ptrbtn,
	.axis = ptraxis,
	.frame = ptrframe
};
static const struct wl_keyboard_listener kblistener = {
	.keymap = kbmap,
	.enter = kbenter,
	.leave = kbleave,
	.key = kbkey,
	.modifiers = kbmodifiers,
	.repeat_info = kbrepeatinfo
};
static const struct zwlr_layer_surface_v1_listener lslistener = {
	.configure = lscfg,
	.closed = lsclosed
};
static const struct wl_seat_listener seatlistener = {
	.capabilities = seatcaps,
	.name = seatname
};
static const struct wl_registry_listener reglistener = {
	.global = regglob,
	.global_remove = regremove
};

static struct wl_compositor *comp;
static struct wl_shm *shm;
static struct zwlr_layer_shell_v1 *shell;
static struct wl_surface *surf;
static struct wl_buffer *buf;
static int scale = DEFAULT_SCALE;
static int grabkbd = DEFAULT_GRAB;
static int testpct = -1;
static int done;

static void
fill(uint32_t *p, int w, int h, int x, int y, int rw, int rh, uint32_t col)
{
	int i, j;
	int x0, x1, y0, y1;

	x0 = x < 0 ? 0 : x;
	y0 = y < 0 ? 0 : y;
	x1 = x + rw > w ? w : x + rw;
	y1 = y + rh > h ? h : y + rh;
	for (j = y0; j < y1; j++) {
		for (i = x0; i < x1; i++)
			p[j * w + i] = col;
	}
}

static void
drawtext(uint32_t *p, int w, int h, int cx, int y, const char *s, int sc, uint32_t col)
{
	int c, r, x;
	size_t i;

	x = cx - ((int)strlen(s) * 6 * sc - sc) / 2;
	for (; *s; s++, x += 6 * sc) {
		for (i = 0; i < sizeof(font) / sizeof(*font); i++) {
			if (font[i].c != *s)
				continue;
			for (r = 0; r < 7; r++) {
				for (c = 0; c < 5; c++) {
					if (font[i].r[r] & (0x10 >> c))
						fill(p, w, h, x + c * sc, y + r * sc, sc, sc, col);
				}
			}
			break;
		}
	}
}

static void
render(uint32_t *p, int w, int h, int cap)
{
	char pct[16];
	int b, cx;
	uint32_t col;

	b = BORDER_WIDTH * scale;
	cx = w / 2;
	col = cap <= LEVEL_CRIT ? COLOR_CRIT_ACCENT : COLOR_LOW_ACCENT;
	fill(p, w, h, 0, 0, w, h, col);
	fill(p, w, h, b, b, w - 2 * b, h - 2 * b, COLOR_BG);
	drawtext(p, w, h, cx, Y_TITLE * scale, cap <= LEVEL_CRIT ? MSG_CRITICAL : MSG_LOW, SCALE_TITLE * scale, COLOR_TEXT);
	snprintf(pct, sizeof(pct), "%d%%", cap);
	drawtext(p, w, h, cx, Y_PERCENT * scale, pct, SCALE_PERCENT * scale, col);
	drawtext(p, w, h, cx, Y_ACTION * scale, MSG_ACTION, SCALE_ACTION * scale, COLOR_TEXT);
}

static int
readsysfs(const char *sub, char *b, size_t sz)
{
	char p[80];
	int fd;
	ssize_t n;

	if (snprintf(p, sizeof(p), "%s/%s", BAT_PATH, sub) >= (int)sizeof(p))
		return -1;
	if ((fd = open(p, O_RDONLY | O_CLOEXEC)) < 0)
		return -1;
	while ((n = read(fd, b, sz - 1)) < 0 && errno == EINTR)
		continue;
	close(fd);
	if (n <= 0)
		return -1;
	while (n > 0 && (b[n - 1] == '\n' || b[n - 1] == '\r'))
		n--;
	b[n] = '\0';
	return 0;
}

static int
readbattery(int *cap, int *dis)
{
	char b[16];
	char *end;
	long val;

	if (readsysfs("status", b, sizeof(b)) < 0)
		return -1;
	*dis = b[0] == 'D';
	if (!*dis) {
		*cap = -1;
		return 0;
	}
	if (readsysfs("capacity", b, sizeof(b)) < 0)
		return -1;
	errno = 0;
	val = strtol(b, &end, 10);
	if (end == b || *end || errno || val < 0 || val > 100)
		return -1;
	*cap = (int)val;
	return 0;
}

static void
regglob(void *d, struct wl_registry *r, uint32_t n, const char *i, uint32_t v)
{
	(void)d;
	(void)v;
	if (!strcmp(i, wl_compositor_interface.name))
		comp = wl_registry_bind(r, n, &wl_compositor_interface, 1);
	else if (!strcmp(i, wl_shm_interface.name))
		shm = wl_registry_bind(r, n, &wl_shm_interface, 1);
	else if (!strcmp(i, zwlr_layer_shell_v1_interface.name))
		shell = wl_registry_bind(r, n, &zwlr_layer_shell_v1_interface, 1);
	else if (!strcmp(i, wl_seat_interface.name))
		wl_seat_add_listener(wl_registry_bind(r, n, &wl_seat_interface, 1), &seatlistener, NULL);
}

static void
regremove(void *d, struct wl_registry *r, uint32_t n)
{
	(void)d;
	(void)r;
	(void)n;
}

static void
seatcaps(void *d, struct wl_seat *s, uint32_t c)
{
	(void)d;
	if (c & WL_SEAT_CAPABILITY_POINTER)
		wl_pointer_add_listener(wl_seat_get_pointer(s), &ptrlistener, NULL);
	if (grabkbd && (c & WL_SEAT_CAPABILITY_KEYBOARD))
		wl_keyboard_add_listener(wl_seat_get_keyboard(s), &kblistener, NULL);
}

static void
seatname(void *d, struct wl_seat *s, const char *name)
{
	(void)d;
	(void)s;
	(void)name;
}

static void
lscfg(void *d, struct zwlr_layer_surface_v1 *l, uint32_t s, uint32_t w, uint32_t h)
{
	(void)d;
	(void)w;
	(void)h;
	zwlr_layer_surface_v1_ack_configure(l, s);
	wl_surface_attach(surf, buf, 0, 0);
	wl_surface_commit(surf);
}

static void
lsclosed(void *d, struct zwlr_layer_surface_v1 *l)
{
	(void)d;
	(void)l;
	done = 1;
}

static void
kbmap(void *d, struct wl_keyboard *k, uint32_t f, int32_t fd, uint32_t sz)
{
	(void)d;
	(void)k;
	(void)f;
	(void)sz;
	close(fd);
}

static void
kbenter(void *d, struct wl_keyboard *k, uint32_t s, struct wl_surface *sf, struct wl_array *keys)
{
	(void)d;
	(void)k;
	(void)s;
	(void)sf;
	(void)keys;
}

static void
kbleave(void *d, struct wl_keyboard *k, uint32_t s, struct wl_surface *sf)
{
	(void)d;
	(void)k;
	(void)s;
	(void)sf;
}

static void
kbkey(void *d, struct wl_keyboard *k, uint32_t s, uint32_t t, uint32_t key, uint32_t st)
{
	(void)d;
	(void)k;
	(void)s;
	(void)t;
	(void)key;
	if (st)
		done = 1;
}

static void
kbmodifiers(void *d, struct wl_keyboard *k, uint32_t s, uint32_t dep, uint32_t lat, uint32_t lck, uint32_t grp)
{
	(void)d;
	(void)k;
	(void)s;
	(void)dep;
	(void)lat;
	(void)lck;
	(void)grp;
}

static void
kbrepeatinfo(void *d, struct wl_keyboard *k, int32_t rate, int32_t delay)
{
	(void)d;
	(void)k;
	(void)rate;
	(void)delay;
}

static void
ptrenter(void *d, struct wl_pointer *p, uint32_t s, struct wl_surface *sf, wl_fixed_t sx, wl_fixed_t sy)
{
	(void)d;
	(void)p;
	(void)s;
	(void)sf;
	(void)sx;
	(void)sy;
}

static void
ptrleave(void *d, struct wl_pointer *p, uint32_t s, struct wl_surface *sf)
{
	(void)d;
	(void)p;
	(void)s;
	(void)sf;
}

static void
ptrmotion(void *d, struct wl_pointer *p, uint32_t t, wl_fixed_t sx, wl_fixed_t sy)
{
	(void)d;
	(void)p;
	(void)t;
	(void)sx;
	(void)sy;
}

static void
ptrbtn(void *d, struct wl_pointer *p, uint32_t s, uint32_t t, uint32_t b, uint32_t st)
{
	(void)d;
	(void)p;
	(void)s;
	(void)t;
	(void)b;
	if (st)
		done = 1;
}

static void
ptraxis(void *d, struct wl_pointer *p, uint32_t t, uint32_t a, wl_fixed_t v)
{
	(void)d;
	(void)p;
	(void)t;
	(void)a;
	(void)v;
}

static void
ptrframe(void *d, struct wl_pointer *p)
{
	(void)d;
	(void)p;
}

static int
showpopup(int cap)
{
	char b[16];
	struct pollfd pfd;
	struct wl_display *dpy;
	struct wl_shm_pool *pool;
	struct zwlr_layer_surface_v1 *ls;
	size_t sz;
	uint32_t *px;
	int fd, h, ret, w;

	ret = -1;
	w = POPUP_WIDTH * scale;
	h = POPUP_HEIGHT * scale;
	sz = (size_t)w * (size_t)h * sizeof(*px);
	done = 0;
	if (!(dpy = wl_display_connect(NULL)))
		return -1;
	wl_registry_add_listener(wl_display_get_registry(dpy), &reglistener, NULL);
	if (wl_display_roundtrip(dpy) < 0 || !comp || !shm || !shell)
		goto disconnect_dpy;
	if ((fd = memfd_create("lowbat", MFD_CLOEXEC)) < 0)
		goto disconnect_dpy;
	if (ftruncate(fd, (off_t)sz) < 0)
		goto close_fd;
	if ((px = mmap(NULL, sz, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0)) == MAP_FAILED)
		goto close_fd;
	render(px, w, h, cap);
	munmap(px, sz);
	if (!(pool = wl_shm_create_pool(shm, fd, (int32_t)sz)))
		goto close_fd;
	buf = wl_shm_pool_create_buffer(pool, 0, w, h, w * (int)sizeof(*px), WL_SHM_FORMAT_ARGB8888);
	wl_shm_pool_destroy(pool);
	close(fd);
	fd = -1;
	if (!buf)
		goto disconnect_dpy;
	if (!(surf = wl_compositor_create_surface(comp)))
		goto destroy_buf;
	if (!(ls = zwlr_layer_shell_v1_get_layer_surface(shell, surf, NULL, ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY, "lowbat")))
		goto destroy_surf;
	zwlr_layer_surface_v1_add_listener(ls, &lslistener, NULL);
	zwlr_layer_surface_v1_set_size(ls, (uint32_t)w, (uint32_t)h);
	zwlr_layer_surface_v1_set_exclusive_zone(ls, -1);
	if (grabkbd)
		zwlr_layer_surface_v1_set_keyboard_interactivity(ls, ZWLR_LAYER_SURFACE_V1_KEYBOARD_INTERACTIVITY_EXCLUSIVE);
	wl_surface_commit(surf);
	pfd.fd = wl_display_get_fd(dpy);
	pfd.events = POLLIN;
	while (!done) {
		while (wl_display_prepare_read(dpy) < 0) {
			if (wl_display_dispatch_pending(dpy) < 0)
				goto destroy_ls;
		}
		if (wl_display_flush(dpy) < 0 && errno != EAGAIN) {
			wl_display_cancel_read(dpy);
			goto destroy_ls;
		}
		if (poll(&pfd, 1, POLL_TIMEOUT) > 0) {
			if (wl_display_read_events(dpy) < 0)
				goto destroy_ls;
		} else {
			wl_display_cancel_read(dpy);
		}
		if (wl_display_dispatch_pending(dpy) < 0)
			goto destroy_ls;
		if (testpct < 0 && readsysfs("status", b, sizeof(b)) >= 0 && b[0] != 'D')
			break;
	}
	ret = 0;
destroy_ls:
	zwlr_layer_surface_v1_destroy(ls);
destroy_surf:
	wl_surface_destroy(surf);
destroy_buf:
	wl_buffer_destroy(buf);
close_fd:
	if (fd >= 0)
		close(fd);
disconnect_dpy:
	wl_display_disconnect(dpy);
	comp = NULL;
	shm = NULL;
	shell = NULL;
	surf = NULL;
	buf = NULL;
	return ret;
}

static void
usage(void)
{
	fprintf(stderr, "usage: %s [-hnv] [-s scale] [-t percent]\n", program_invocation_short_name);
	exit(1);
}

int
main(int argc, char *argv[])
{
	char *end;
	int cap, dis, opt, shown, want;
	long val;

	shown = 101;
	while ((opt = getopt(argc, argv, "hns:t:v")) >= 0) {
		switch (opt) {
		case 'h':
			usage();
			break;
		case 'n':
			grabkbd = 0;
			break;
		case 's':
			errno = 0;
			val = strtol(optarg, &end, 10);
			if (!*optarg || *end || errno || val < 1 || val > 16)
				usage();
			scale = (int)val;
			break;
		case 't':
			errno = 0;
			val = strtol(optarg, &end, 10);
			if (!*optarg || *end || errno || val < 0 || val > 100)
				usage();
			testpct = (int)val;
			break;
		case 'v':
			puts("lowbat-" VERSION);
			return 0;
		default:
			usage();
			break;
		}
	}
	argc -= optind;
	if (argc)
		usage();
	if (testpct >= 0)
		return showpopup(testpct) < 0;
	for (;;) {
		if (readbattery(&cap, &dis) >= 0) {
			if (!dis) {
				shown = 101;
			} else {
				if (cap <= LEVEL_CRIT)
					want = LEVEL_CRIT;
				else if (cap <= LEVEL_LOW)
					want = LEVEL_LOW;
				else
					want = 0;
				if (want && want < shown && showpopup(cap) >= 0)
					shown = want;
			}
		}
		sleep((unsigned int)POLL_INTERVAL);
	}
}
