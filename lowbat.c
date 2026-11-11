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

static void draw_text(uint32_t *p, int w, int h, int cx, int y, const char *s, int sc, uint32_t col);
static void fill(uint32_t *p, int w, int h, int x, int y, int rw, int rh, uint32_t col);
static void kb_enter(void *d, struct wl_keyboard *k, uint32_t s, struct wl_surface *surf, struct wl_array *keys);
static void kb_key(void *d, struct wl_keyboard *k, uint32_t s, uint32_t t, uint32_t key, uint32_t st);
static void kb_leave(void *d, struct wl_keyboard *k, uint32_t s, struct wl_surface *surf);
static void kb_map(void *d, struct wl_keyboard *k, uint32_t f, int32_t fd, uint32_t sz);
static void kb_modifiers(void *d, struct wl_keyboard *k, uint32_t s, uint32_t dep, uint32_t lat, uint32_t lck, uint32_t grp);
static void kb_repeat_info(void *d, struct wl_keyboard *k, int32_t rate, int32_t delay);
static void ls_cfg(void *d, struct zwlr_layer_surface_v1 *l, uint32_t s, uint32_t w, uint32_t h);
static void ls_closed(void *d, struct zwlr_layer_surface_v1 *l);
static void ptr_axis(void *d, struct wl_pointer *p, uint32_t t, uint32_t a, wl_fixed_t v);
static void ptr_btn(void *d, struct wl_pointer *p, uint32_t s, uint32_t t, uint32_t b, uint32_t st);
static void ptr_enter(void *d, struct wl_pointer *p, uint32_t s, struct wl_surface *surf, wl_fixed_t sx, wl_fixed_t sy);
static void ptr_frame(void *d, struct wl_pointer *p);
static void ptr_leave(void *d, struct wl_pointer *p, uint32_t s, struct wl_surface *surf);
static void ptr_motion(void *d, struct wl_pointer *p, uint32_t t, wl_fixed_t sx, wl_fixed_t sy);
static int read_battery(int *cap, int *dis);
static int read_sysfs(const char *sub, char *b, size_t sz);
static void reg_glob(void *d, struct wl_registry *r, uint32_t n, const char *i, uint32_t v);
static void reg_remove(void *d, struct wl_registry *r, uint32_t n);
static void render(uint32_t *p, int w, int h, int cap);
static void seat_caps(void *d, struct wl_seat *s, uint32_t c);
static void seat_name(void *d, struct wl_seat *s, const char *name);
static int show_popup(int cap);
static void usage(void);

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

static const struct wl_pointer_listener ptr_l = {
	.enter = ptr_enter,
	.leave = ptr_leave,
	.motion = ptr_motion,
	.button = ptr_btn,
	.axis = ptr_axis,
	.frame = ptr_frame
};
static const struct wl_keyboard_listener kb_l = {
	.keymap = kb_map,
	.enter = kb_enter,
	.leave = kb_leave,
	.key = kb_key,
	.modifiers = kb_modifiers,
	.repeat_info = kb_repeat_info
};
static const struct zwlr_layer_surface_v1_listener ls_l = {
	.configure = ls_cfg,
	.closed = ls_closed
};
static const struct wl_seat_listener seat_l = {
	.capabilities = seat_caps,
	.name = seat_name
};
static const struct wl_registry_listener reg_l = {
	.global = reg_glob,
	.global_remove = reg_remove
};

const struct wl_interface xdg_popup_interface;

static struct wl_compositor *comp;
static struct wl_shm *shm;
static struct zwlr_layer_shell_v1 *shell;
static struct wl_surface *surf;
static struct wl_buffer *buf;
static char *argv0;
static int scale = default_scale;
static int grab_kbd = default_grab;
static int test_pct = -1;
static int done;

static void
draw_text(uint32_t *p, int w, int h, int cx, int y, const char *s, int sc, uint32_t col)
{
	int c;
	int r;
	int x;
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
fill(uint32_t *p, int w, int h, int x, int y, int rw, int rh, uint32_t col)
{
	int i;
	int j;
	int x0;
	int x1;
	int y0;
	int y1;

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
kb_enter(void *d, struct wl_keyboard *k, uint32_t s, struct wl_surface *surf, struct wl_array *keys)
{
	(void)d;
	(void)k;
	(void)s;
	(void)surf;
	(void)keys;
}

static void
kb_key(void *d, struct wl_keyboard *k, uint32_t s, uint32_t t, uint32_t key, uint32_t st)
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
kb_leave(void *d, struct wl_keyboard *k, uint32_t s, struct wl_surface *surf)
{
	(void)d;
	(void)k;
	(void)s;
	(void)surf;
}

static void
kb_map(void *d, struct wl_keyboard *k, uint32_t f, int32_t fd, uint32_t sz)
{
	(void)d;
	(void)k;
	(void)f;
	(void)sz;
	close(fd);
}

static void
kb_modifiers(void *d, struct wl_keyboard *k, uint32_t s, uint32_t dep, uint32_t lat, uint32_t lck, uint32_t grp)
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
kb_repeat_info(void *d, struct wl_keyboard *k, int32_t rate, int32_t delay)
{
	(void)d;
	(void)k;
	(void)rate;
	(void)delay;
}

static void
ls_cfg(void *d, struct zwlr_layer_surface_v1 *l, uint32_t s, uint32_t w, uint32_t h)
{
	(void)d;
	(void)w;
	(void)h;
	zwlr_layer_surface_v1_ack_configure(l, s);
	wl_surface_attach(surf, buf, 0, 0);
	wl_surface_commit(surf);
}

static void
ls_closed(void *d, struct zwlr_layer_surface_v1 *l)
{
	(void)d;
	(void)l;
	done = 1;
}

static void
ptr_axis(void *d, struct wl_pointer *p, uint32_t t, uint32_t a, wl_fixed_t v)
{
	(void)d;
	(void)p;
	(void)t;
	(void)a;
	(void)v;
}

static void
ptr_btn(void *d, struct wl_pointer *p, uint32_t s, uint32_t t, uint32_t b, uint32_t st)
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
ptr_enter(void *d, struct wl_pointer *p, uint32_t s, struct wl_surface *surf, wl_fixed_t sx, wl_fixed_t sy)
{
	(void)d;
	(void)p;
	(void)s;
	(void)surf;
	(void)sx;
	(void)sy;
}

static void
ptr_frame(void *d, struct wl_pointer *p)
{
	(void)d;
	(void)p;
}

static void
ptr_leave(void *d, struct wl_pointer *p, uint32_t s, struct wl_surface *surf)
{
	(void)d;
	(void)p;
	(void)s;
	(void)surf;
}

static void
ptr_motion(void *d, struct wl_pointer *p, uint32_t t, wl_fixed_t sx, wl_fixed_t sy)
{
	(void)d;
	(void)p;
	(void)t;
	(void)sx;
	(void)sy;
}

static int
read_battery(int *cap, int *dis)
{
	char b[16];
	char *end;
	long val;

	if (read_sysfs("status", b, sizeof(b)) < 0)
		return -1;
	*dis = (b[0] == 'D');
	if (!*dis)
		return 0;
	if (read_sysfs("capacity", b, sizeof(b)) < 0)
		return -1;
	errno = 0;
	val = strtol(b, &end, 10);
	if (end == b || errno != 0 || val < 0 || val > 100)
		return -1;
	*cap = (int)val;
	return 0;
}

static int
read_sysfs(const char *sub, char *b, size_t sz)
{
	char p[80];
	int fd;
	ssize_t n;

	snprintf(p, sizeof(p), "%s/%s", bat_path, sub);
	if ((fd = open(p, O_RDONLY | O_CLOEXEC)) < 0)
		return -1;
	while ((n = read(fd, b, sz - 1)) < 0 && errno == EINTR)
		continue;
	close(fd);
	if (n <= 0)
		return -1;
	b[n] = '\0';
	return 0;
}

static void
reg_glob(void *d, struct wl_registry *r, uint32_t n, const char *i, uint32_t v)
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
		wl_seat_add_listener(wl_registry_bind(r, n, &wl_seat_interface, 1), &seat_l, NULL);
}

static void
reg_remove(void *d, struct wl_registry *r, uint32_t n)
{
	(void)d;
	(void)r;
	(void)n;
}

static void
render(uint32_t *p, int w, int h, int cap)
{
	char pct[16];
	int b;
	int c;
	int cx;
	uint32_t col;

	b = border_width * scale;
	cx = w / 2;
	c = cap <= level_crit;
	col = c ? color_crit_accent : color_low_accent;
	fill(p, w, h, 0, 0, w, h, col);
	fill(p, w, h, b, b, w - 2 * b, h - 2 * b, color_bg);
	draw_text(p, w, h, cx, y_title * scale, c ? msg_critical : msg_low, scale_title * scale, color_text);
	snprintf(pct, sizeof(pct), "%d%%", cap);
	draw_text(p, w, h, cx, y_percent * scale, pct, scale_percent * scale, col);
	draw_text(p, w, h, cx, y_action * scale, msg_action, scale_action * scale, color_text);
}

static void
seat_caps(void *d, struct wl_seat *s, uint32_t c)
{
	(void)d;
	if (c & WL_SEAT_CAPABILITY_POINTER)
		wl_pointer_add_listener(wl_seat_get_pointer(s), &ptr_l, NULL);
	if (grab_kbd && (c & WL_SEAT_CAPABILITY_KEYBOARD))
		wl_keyboard_add_listener(wl_seat_get_keyboard(s), &kb_l, NULL);
}

static void
seat_name(void *d, struct wl_seat *s, const char *name)
{
	(void)d;
	(void)s;
	(void)name;
}

static int
show_popup(int cap)
{
	char b[16];
	struct pollfd pfd;
	struct wl_display *dpy;
	struct wl_shm_pool *pool;
	struct zwlr_layer_surface_v1 *ls;
	size_t sz;
	uint32_t *px;
	int fd;
	int h;
	int ret;
	int w;

	ret = -1;
	w = popup_width * scale;
	h = popup_height * scale;
	sz = (size_t)w * 4 * h;
	done = 0;
	if (!(dpy = wl_display_connect(NULL)))
		return -1;
	wl_registry_add_listener(wl_display_get_registry(dpy), &reg_l, NULL);
	wl_display_roundtrip(dpy);
	if (!comp || !shm || !shell)
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
	buf = wl_shm_pool_create_buffer(pool, 0, w, h, w * 4, WL_SHM_FORMAT_ARGB8888);
	wl_shm_pool_destroy(pool);
	close(fd);
	fd = -1;
	if (!buf)
		goto disconnect_dpy;
	if (!(surf = wl_compositor_create_surface(comp)))
		goto destroy_buf;
	if (!(ls = zwlr_layer_shell_v1_get_layer_surface(shell, surf, NULL, ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY, "lowbat")))
		goto destroy_surf;
	zwlr_layer_surface_v1_add_listener(ls, &ls_l, NULL);
	zwlr_layer_surface_v1_set_size(ls, (uint32_t)w, (uint32_t)h);
	zwlr_layer_surface_v1_set_exclusive_zone(ls, -1);
	if (grab_kbd)
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
		if (poll(&pfd, 1, poll_timeout) > 0) {
			if (wl_display_read_events(dpy) < 0)
				goto destroy_ls;
		} else {
			wl_display_cancel_read(dpy);
		}
		if (wl_display_dispatch_pending(dpy) < 0)
			goto destroy_ls;
		if (test_pct < 0 && !read_sysfs("status", b, sizeof(b)) && b[0] != 'D')
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
	fprintf(stderr, "usage: %s [-hnv] [-s scale] [-t percent]\n", argv0);
	exit(1);
}

int
main(int argc, char *argv[])
{
	char *end;
	int cap;
	int dis;
	int opt;
	int shown;
	int want;
	long val;

	argv0 = argv[0];
	shown = 101;
	while ((opt = getopt(argc, argv, "hns:t:v")) != -1) {
		switch (opt) {
		case 'h':
			usage();
			break;
		case 'n':
			grab_kbd = 0;
			break;
		case 's':
			errno = 0;
			val = strtol(optarg, &end, 10);
			if (*optarg == '\0' || *end != '\0' || errno != 0 || val < 1 || val > 16)
				usage();
			scale = (int)val;
			break;
		case 't':
			errno = 0;
			val = strtol(optarg, &end, 10);
			if (*optarg == '\0' || *end != '\0' || errno != 0 || val < 0 || val > 100)
				usage();
			test_pct = (int)val;
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
	argv += optind;
	if (argc > 0)
		usage();
	if (test_pct >= 0)
		return show_popup(test_pct) < 0 ? 1 : 0;
	for (;;) {
		if (read_battery(&cap, &dis) < 0) {
			sleep((unsigned int)poll_interval);
			continue;
		}
		if (!dis) {
			shown = 101;
		} else {
			if (cap <= level_crit)
				want = level_crit;
			else if (cap <= level_low)
				want = level_low;
			else
				want = 0;
			if (want && want < shown && !show_popup(cap))
				shown = want;
		}
		sleep((unsigned int)poll_interval);
	}
}
