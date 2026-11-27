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
#ifndef DMOST_SDL_FONT_H
#define DMOST_SDL_FONT_H

#include <SDL.h>

#define SDL_FONT_GLYPH_W 8
#define SDL_FONT_GLYPH_H 16

enum sdl_text_align {
	SDL_ALIGN_LEFT = 0,
	SDL_ALIGN_CENTER = 1,
	SDL_ALIGN_RIGHT = 2,
};

struct sdl_font {
	SDL_Texture *atlas_nearest;
	SDL_Texture *atlas_linear;
};

int sdl_font_init(struct sdl_font *font, SDL_Renderer *renderer);
void sdl_font_destroy(struct sdl_font *font);

float sdl_font_measure_width(const char *text, float pixel_h);

void sdl_font_draw(SDL_Renderer *renderer, const struct sdl_font *font,
		   float x, float cy, float pixel_h, const char *text,
		   enum sdl_text_align align, SDL_Color color);

/*
 * Word-wraps text to max_w and draws lines starting at line_offset (up to
 * max_lines if max_lines > 0). Returns the total number of wrapped lines.
 * Passing renderer == NULL or font == NULL measures line count without drawing.
 */
int sdl_font_draw_wrapped(SDL_Renderer *renderer, const struct sdl_font *font,
			  float x, float y, float max_w, float pixel_h,
			  float line_spacing, const char *text,
			  SDL_Color color, int line_offset, int max_lines);

#endif /* DMOST_SDL_FONT_H */
