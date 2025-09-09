//SPDX-License-Identifier: GPL-2.0-or-later
/*

   Copyright (c) 2019-2025 Cyril Hrubis <metan@ucw.cz>

 */

#include <gfx/gp_gfx.h>
#include "neko_logo.h"

struct rgb {
	uint8_t r, g, b;
};

struct rgb light_rgb_palette[] = {
	{0x56, 0x7d, 0x9e},
	{0x00, 0x00, 0x00},
	{0x91, 0x91, 0x91},
	{0x73, 0x65, 0x56},
	{0x83, 0x23, 0x23},
};

struct rgb dark_rgb_palette[] = {
	{0x22, 0x22, 0x2f},
	{0xff, 0xff, 0xff},
	{0x91, 0x91, 0x91},
	{0x73, 0x65, 0x56},
	{0x83, 0x23, 0x23},
};

struct rgb dark_1bpp_palette[] = {
	{0x00, 0x00, 0x00},
	{0xff, 0xff, 0xff},
	{0x00, 0x00, 0x00},
	{0x00, 0x00, 0x00},
	{0xff, 0xff, 0xff},
};

struct rgb dark_2bpp_palette[] = {
	{0x00, 0x00, 0x00},
	{0xff, 0xff, 0xff},
	{0x40, 0x40, 0x40},
	{0x40, 0x40, 0x40},
	{0x80, 0x80, 0x80},
};

struct rgb light_1bpp_palette[] = {
	{0xff, 0xff, 0xff},
	{0x00, 0x00, 0x00},
	{0xff, 0xff, 0xff},
	{0xff, 0xff, 0xff},
	{0x00, 0x00, 0x00},
};

struct rgb light_2bpp_palette[] = {
	{0xff, 0xff, 0xff},
	{0x00, 0x00, 0x00},
	{0x80, 0x80, 0x80},
	{0x80, 0x80, 0x80},
	{0x40, 0x40, 0x40},
};

static gp_pixel *get_palette(gp_pixel_type pixel_type, int dark)
{
	gp_pixel *ret = malloc(sizeof(gp_pixel) * 5);
	size_t i;
	struct rgb *palette = dark ? dark_rgb_palette : light_rgb_palette;

	switch (gp_pixel_size(pixel_type)) {
	case 1:
		if (dark)
			palette = dark_1bpp_palette;
		else
			palette = light_1bpp_palette;
	break;
	case 2:
		if (dark)
			palette = dark_2bpp_palette;
		else
			palette = light_2bpp_palette;
	break;
	}

	for (i = 0; i < 5; i++) {
		ret[i] = gp_rgb_to_pixel(palette[i].r, palette[i].g, palette[i].b, pixel_type);
	}

	return ret;
}

void neko_logo_render(gp_pixmap *pixmap, struct neko_logo *logo, gp_size y_off, int dark_theme)
{
	gp_coord cx = gp_pixmap_w(pixmap)/2;
	gp_coord cy = gp_pixmap_h(pixmap)/2;
	gp_size s = GP_MIN(cx, cy);
	gp_size pix_size = s/GP_MAX(logo->w, logo->h);
	gp_pixel *palette;
	gp_coord x, y;

	palette = get_palette(pixmap->pixel_type, dark_theme);
	if (!palette)
		return;

	gp_fill(pixmap, palette[0]);

	cx -= pix_size * logo->w/2;
	cy -= pix_size * logo->h/2 + y_off;

	for (y = 0; y < logo->h; y++) {
		for (x = 0; x < logo->w; x++) {
			uint32_t i = y * logo->w + x;
			gp_fill_rect_xywh(pixmap, cx + x * pix_size, cy + y * pix_size, pix_size, pix_size, palette[logo->data[i]]);
		}
	}

	free(palette);
}
