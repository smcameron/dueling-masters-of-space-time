/*
 *  (C) Copyright 2026 Stephen M. Cameron.
 *
 *  This file is part of Dueling Masters of Space Time.
 *
 *  Dueling Masters of Space Time is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Dueling Masters of Space Time is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Dueling Masters of Space Time; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */
#include <math.h>
#include <png.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sdl_gfx.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define MAX_CIRCLE_SEGS 72

static Uint8 clamp_u8(double v)
{
	if (v <= 0.0)
		return 0;
	if (v >= 1.0)
		return 255;
	return (Uint8)(v * 255.0 + 0.5);
}

SDL_Color sdl_rgba_f(double r, double g, double b, double a)
{
	SDL_Color c;

	c.r = clamp_u8(r);
	c.g = clamp_u8(g);
	c.b = clamp_u8(b);
	c.a = clamp_u8(a);
	return c;
}

struct png_load_state {
	FILE *fp;
	png_structp png;
	png_infop info;
	png_bytep *rows;
	SDL_Surface *surf;
};

static void png_load_cleanup(struct png_load_state *st)
{
	if (st->rows != NULL) {
		free(st->rows);
		st->rows = NULL;
	}
	if (st->surf != NULL) {
		SDL_FreeSurface(st->surf);
		st->surf = NULL;
	}
	if (st->png != NULL) {
		png_destroy_read_struct(&st->png,
					st->info ? &st->info : NULL, NULL);
		st->png = NULL;
		st->info = NULL;
	}
	if (st->fp != NULL) {
		fclose(st->fp);
		st->fp = NULL;
	}
}

SDL_Texture *sdl_load_png_texture(SDL_Renderer *renderer, const char *path,
				  int *out_w, int *out_h)
{
	struct png_load_state st;
	uint8_t sig[8];
	png_uint_32 w, h, row_idx;
	int bit_depth, color_type;
	SDL_Texture *tex;

	if (renderer == NULL || path == NULL)
		return NULL;

	memset(&st, 0, sizeof(st));
	st.fp = fopen(path, "rb");
	if (st.fp == NULL)
		return NULL;

	if (fread(sig, 1, sizeof(sig), st.fp) != sizeof(sig) ||
	    png_sig_cmp(sig, 0, sizeof(sig)) != 0) {
		png_load_cleanup(&st);
		return NULL;
	}

	st.png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (st.png == NULL) {
		png_load_cleanup(&st);
		return NULL;
	}

	st.info = png_create_info_struct(st.png);
	if (st.info == NULL) {
		png_load_cleanup(&st);
		return NULL;
	}

	if (setjmp(png_jmpbuf(st.png))) {
		png_load_cleanup(&st);
		return NULL;
	}

	png_init_io(st.png, st.fp);
	png_set_sig_bytes(st.png, (int)sizeof(sig));
	png_read_info(st.png, st.info);

	w = png_get_image_width(st.png, st.info);
	h = png_get_image_height(st.png, st.info);
	bit_depth = png_get_bit_depth(st.png, st.info);
	color_type = png_get_color_type(st.png, st.info);

	if (w == 0 || h == 0) {
		png_load_cleanup(&st);
		return NULL;
	}

	if (bit_depth == 16)
		png_set_strip_16(st.png);
	if (color_type == PNG_COLOR_TYPE_PALETTE)
		png_set_palette_to_rgb(st.png);
	if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
		png_set_expand_gray_1_2_4_to_8(st.png);
	if (png_get_valid(st.png, st.info, PNG_INFO_tRNS))
		png_set_tRNS_to_alpha(st.png);
	if (color_type == PNG_COLOR_TYPE_GRAY ||
	    color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
		png_set_gray_to_rgb(st.png);
	if (color_type == PNG_COLOR_TYPE_RGB ||
	    color_type == PNG_COLOR_TYPE_GRAY ||
	    color_type == PNG_COLOR_TYPE_PALETTE)
		png_set_filler(st.png, 0xFF, PNG_FILLER_AFTER);

	png_read_update_info(st.png, st.info);

	st.surf = SDL_CreateRGBSurfaceWithFormat(0, (int)w, (int)h, 32,
						 SDL_PIXELFORMAT_RGBA32);
	if (st.surf == NULL) {
		png_load_cleanup(&st);
		return NULL;
	}

	st.rows = malloc(sizeof(png_bytep) * h);
	if (st.rows == NULL) {
		png_load_cleanup(&st);
		return NULL;
	}

	if (SDL_MUSTLOCK(st.surf))
		SDL_LockSurface(st.surf);
	for (row_idx = 0; row_idx < h; row_idx++)
		st.rows[row_idx] = (png_bytep)st.surf->pixels +
				   row_idx * (png_uint_32)st.surf->pitch;

	png_read_image(st.png, st.rows);
	png_read_end(st.png, NULL);
	if (SDL_MUSTLOCK(st.surf))
		SDL_UnlockSurface(st.surf);

	tex = SDL_CreateTextureFromSurface(renderer, st.surf);
	if (tex != NULL) {
		SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
		if (out_w != NULL)
			*out_w = (int)w;
		if (out_h != NULL)
			*out_h = (int)h;
	}

	png_load_cleanup(&st);
	return tex;
}

void sdl_gfx_fill_rect(SDL_Renderer *renderer, float x, float y,
		       float w, float h, SDL_Color color)
{
	SDL_FRect r;

	if (renderer == NULL || w <= 0.0f || h <= 0.0f)
		return;

	r.x = x;
	r.y = y;
	r.w = w;
	r.h = h;
	SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
	SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
	SDL_RenderFillRectF(renderer, &r);
}

void sdl_gfx_stroke_rect(SDL_Renderer *renderer, float x, float y,
			 float w, float h, float line_width, SDL_Color color)
{
	float lw = (line_width > 0.0f) ? line_width : 1.0f;
	float half = lw * 0.5f;

	if (renderer == NULL || w <= 0.0f || h <= 0.0f)
		return;

	/* Top & bottom full-width edges, left & right inner vertical edges */
	sdl_gfx_fill_rect(renderer, x - half, y - half, w + lw, lw, color);
	sdl_gfx_fill_rect(renderer, x - half, y + h - half, w + lw, lw, color);
	if (h > lw) {
		sdl_gfx_fill_rect(renderer, x - half, y + half, lw, h - lw, color);
		sdl_gfx_fill_rect(renderer, x + w - half, y + half, lw, h - lw, color);
	}
}

static int circle_segment_count(float radius)
{
	int segs = (int)(radius * 1.5f);

	if (segs < 20)
		segs = 20;
	if (segs > MAX_CIRCLE_SEGS)
		segs = MAX_CIRCLE_SEGS;
	return segs;
}

void sdl_gfx_fill_circle(SDL_Renderer *renderer, float cx, float cy,
			 float radius, SDL_Color color)
{
	SDL_Vertex verts[MAX_CIRCLE_SEGS + 2];
	int indices[MAX_CIRCLE_SEGS * 3];
	int segs, i;

	if (renderer == NULL || radius <= 0.0f)
		return;

	segs = circle_segment_count(radius);
	verts[0].position.x = cx;
	verts[0].position.y = cy;
	verts[0].color = color;
	verts[0].tex_coord.x = 0.0f;
	verts[0].tex_coord.y = 0.0f;

	for (i = 0; i <= segs; i++) {
		double ang = (2.0 * M_PI * (double)i) / (double)segs;

		verts[i + 1].position.x = cx + (float)cos(ang) * radius;
		verts[i + 1].position.y = cy + (float)sin(ang) * radius;
		verts[i + 1].color = color;
		verts[i + 1].tex_coord.x = 0.0f;
		verts[i + 1].tex_coord.y = 0.0f;
	}

	for (i = 0; i < segs; i++) {
		indices[i * 3 + 0] = 0;
		indices[i * 3 + 1] = i + 1;
		indices[i * 3 + 2] = i + 2;
	}

	SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
	SDL_RenderGeometry(renderer, NULL, verts, segs + 2,
			   indices, segs * 3);
}

static void draw_arc_ring_strip(SDL_Renderer *renderer, float cx, float cy,
				float r_in, float r_out,
				double a0, double a1, int segs,
				SDL_Color color)
{
	SDL_Vertex verts[(MAX_CIRCLE_SEGS + 1) * 2];
	int indices[MAX_CIRCLE_SEGS * 6];
	int i;

	if (segs < 1)
		segs = 1;
	if (segs > MAX_CIRCLE_SEGS)
		segs = MAX_CIRCLE_SEGS;

	for (i = 0; i <= segs; i++) {
		double ang = a0 + (a1 - a0) * ((double)i / (double)segs);
		float cs = (float)cos(ang);
		float sn = (float)sin(ang);

		verts[i * 2 + 0].position.x = cx + cs * r_in;
		verts[i * 2 + 0].position.y = cy + sn * r_in;
		verts[i * 2 + 0].color = color;
		verts[i * 2 + 0].tex_coord.x = 0.0f;
		verts[i * 2 + 0].tex_coord.y = 0.0f;

		verts[i * 2 + 1].position.x = cx + cs * r_out;
		verts[i * 2 + 1].position.y = cy + sn * r_out;
		verts[i * 2 + 1].color = color;
		verts[i * 2 + 1].tex_coord.x = 0.0f;
		verts[i * 2 + 1].tex_coord.y = 0.0f;
	}

	for (i = 0; i < segs; i++) {
		int v0 = i * 2;
		int v1 = i * 2 + 1;
		int v2 = (i + 1) * 2;
		int v3 = (i + 1) * 2 + 1;

		indices[i * 6 + 0] = v0;
		indices[i * 6 + 1] = v1;
		indices[i * 6 + 2] = v3;
		indices[i * 6 + 3] = v0;
		indices[i * 6 + 4] = v3;
		indices[i * 6 + 5] = v2;
	}

	SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
	SDL_RenderGeometry(renderer, NULL, verts, (segs + 1) * 2,
			   indices, segs * 6);
}

void sdl_gfx_stroke_circle(SDL_Renderer *renderer, float cx, float cy,
			   float radius, float line_width, SDL_Color color,
			   float dash_len, float gap_len)
{
	float lw = (line_width > 0.0f) ? line_width : 1.0f;
	float r_in = radius - lw * 0.5f;
	float r_out = radius + lw * 0.5f;

	if (renderer == NULL || radius <= 0.0f)
		return;
	if (r_in < 0.0f)
		r_in = 0.0f;

	if (dash_len <= 0.0f || gap_len <= 0.0f) {
		int segs = circle_segment_count(radius);
		draw_arc_ring_strip(renderer, cx, cy, r_in, r_out,
				    0.0, 2.0 * M_PI, segs, color);
		return;
	}

	{
		double dash_ang = (double)dash_len / (double)radius;
		double gap_ang = (double)gap_len / (double)radius;
		double step = dash_ang + gap_ang;
		double ang = 0.0;

		if (step < 0.02)
			step = 0.02;

		while (ang < 2.0 * M_PI) {
			double end_ang = ang + dash_ang;
			int segs;

			if (end_ang > 2.0 * M_PI)
				end_ang = 2.0 * M_PI;
			segs = (int)((end_ang - ang) * (double)radius / 3.0) + 1;
			if (segs < 2)
				segs = 2;
			if (segs > 16)
				segs = 16;

			draw_arc_ring_strip(renderer, cx, cy, r_in, r_out,
					    ang, end_ang, segs, color);
			ang += step;
		}
	}
}

void sdl_gfx_thick_line(SDL_Renderer *renderer, float x1, float y1,
			float x2, float y2, float line_width,
			SDL_Color color, int round_caps)
{
	float dx = x2 - x1;
	float dy = y2 - y1;
	float len = sqrtf(dx * dx + dy * dy);
	float lw = (line_width > 0.0f) ? line_width : 1.0f;
	float nx, ny;
	SDL_Vertex verts[4];
	int indices[6] = { 0, 1, 2, 0, 2, 3 };

	if (renderer == NULL)
		return;

	if (len < 1e-4f) {
		if (round_caps)
			sdl_gfx_fill_circle(renderer, x1, y1, lw * 0.5f, color);
		return;
	}

	nx = (-dy / len) * (lw * 0.5f);
	ny = (dx / len) * (lw * 0.5f);

	verts[0].position.x = x1 + nx;
	verts[0].position.y = y1 + ny;
	verts[1].position.x = x1 - nx;
	verts[1].position.y = y1 - ny;
	verts[2].position.x = x2 - nx;
	verts[2].position.y = y2 - ny;
	verts[3].position.x = x2 + nx;
	verts[3].position.y = y2 + ny;

	for (int i = 0; i < 4; i++) {
		verts[i].color = color;
		verts[i].tex_coord.x = 0.0f;
		verts[i].tex_coord.y = 0.0f;
	}

	SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
	SDL_RenderGeometry(renderer, NULL, verts, 4, indices, 6);

	if (round_caps && lw > 2.0f) {
		sdl_gfx_fill_circle(renderer, x1, y1, lw * 0.5f, color);
		sdl_gfx_fill_circle(renderer, x2, y2, lw * 0.5f, color);
	}
}

void sdl_gfx_polyline(SDL_Renderer *renderer, const SDL_FPoint *pts,
		      int count, float line_width, SDL_Color color)
{
	int i;
	float lw = (line_width > 0.0f) ? line_width : 1.0f;

	if (renderer == NULL || pts == NULL || count <= 0)
		return;

	if (count == 1) {
		sdl_gfx_fill_circle(renderer, pts[0].x, pts[0].y,
				    lw * 0.5f, color);
		return;
	}

	for (i = 0; i < count - 1; i++) {
		sdl_gfx_thick_line(renderer, pts[i].x, pts[i].y,
				   pts[i + 1].x, pts[i + 1].y,
				   lw, color, 0);
	}

	if (lw > 2.0f) {
		for (i = 0; i < count; i++)
			sdl_gfx_fill_circle(renderer, pts[i].x, pts[i].y,
					    lw * 0.5f, color);
	}
}

void sdl_gfx_dashed_line(SDL_Renderer *renderer, float x1, float y1,
			 float x2, float y2, float line_width,
			 float dash_len, float gap_len, SDL_Color color)
{
	float dx = x2 - x1;
	float dy = y2 - y1;
	float len = sqrtf(dx * dx + dy * dy);
	float ux, uy, pos = 0.0f;

	if (renderer == NULL || len < 1e-4f)
		return;
	if (dash_len <= 0.0f || gap_len <= 0.0f) {
		sdl_gfx_thick_line(renderer, x1, y1, x2, y2,
				   line_width, color, 0);
		return;
	}

	ux = dx / len;
	uy = dy / len;

	while (pos < len) {
		float seg_end = pos + dash_len;

		if (seg_end > len)
			seg_end = len;
		sdl_gfx_thick_line(renderer,
				   x1 + ux * pos, y1 + uy * pos,
				   x1 + ux * seg_end, y1 + uy * seg_end,
				   line_width, color, 0);
		pos += dash_len + gap_len;
	}
}
