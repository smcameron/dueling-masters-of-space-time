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
#ifndef DMOST_SDL_GFX_H
#define DMOST_SDL_GFX_H

#include <SDL.h>

SDL_Color sdl_rgba_f(double r, double g, double b, double a);

SDL_Texture *sdl_load_png_texture(SDL_Renderer *renderer, const char *path,
				  int *out_w, int *out_h);

void sdl_gfx_fill_rect(SDL_Renderer *renderer, float x, float y,
		       float w, float h, SDL_Color color);

void sdl_gfx_stroke_rect(SDL_Renderer *renderer, float x, float y,
			 float w, float h, float line_width, SDL_Color color);

void sdl_gfx_fill_circle(SDL_Renderer *renderer, float cx, float cy,
			 float radius, SDL_Color color);

/*
 * Draws a circle outline centered at (cx, cy). If dash_len > 0 and gap_len > 0,
 * draws a dashed ring; otherwise draws a continuous ring.
 */
void sdl_gfx_stroke_circle(SDL_Renderer *renderer, float cx, float cy,
			   float radius, float line_width, SDL_Color color,
			   float dash_len, float gap_len);

void sdl_gfx_thick_line(SDL_Renderer *renderer, float x1, float y1,
			float x2, float y2, float line_width,
			SDL_Color color, int round_caps);

void sdl_gfx_polyline(SDL_Renderer *renderer, const SDL_FPoint *pts,
		      int count, float line_width, SDL_Color color);

void sdl_gfx_dashed_line(SDL_Renderer *renderer, float x1, float y1,
			 float x2, float y2, float line_width,
			 float dash_len, float gap_len, SDL_Color color);

#endif /* DMOST_SDL_GFX_H */
