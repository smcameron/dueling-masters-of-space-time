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
#ifndef DMOST_SDL_UI_H
#define DMOST_SDL_UI_H

#include <SDL.h>

#include "sdl_font.h"

#define SDL_UI_FONT_H 16.0f
#define SDL_LOG_MAX_ENTRIES 256
#define SDL_LOG_ENTRY_MAX_LEN 256

int sdl_point_in_rect(float x, float y, SDL_FRect r);

struct sdl_button {
	SDL_FRect rect;
	const char *label;
	const char *tooltip;
	int visible;
	int enabled;
	int accent;
	int hovered;
	int pressed;
};

void sdl_button_init(struct sdl_button *btn, const char *label,
		     const char *tooltip);
float sdl_button_preferred_width(const char *label);
int sdl_button_handle_motion(struct sdl_button *btn, float mx, float my);
int sdl_button_handle_mouse_down(struct sdl_button *btn, float mx, float my);
int sdl_button_handle_mouse_up(struct sdl_button *btn, float mx, float my,
			       int *clicked);
void sdl_button_draw(SDL_Renderer *renderer, const struct sdl_font *font,
		     const struct sdl_button *btn);

struct sdl_dropdown {
	SDL_FRect rect;
	const char *const *items;
	int item_count;
	int active_index;
	const char *tooltip;
	int enabled;
	int open;
	int hovered;
	int hover_index;
};

void sdl_dropdown_init(struct sdl_dropdown *dd, const char *const *items,
		       int item_count, int active_index, const char *tooltip);
float sdl_dropdown_preferred_width(const char *const *items, int item_count);
SDL_FRect sdl_dropdown_popup_rect(const struct sdl_dropdown *dd, int win_w);
int sdl_dropdown_handle_motion(struct sdl_dropdown *dd, float mx, float my,
			       int win_w);
int sdl_dropdown_handle_click(struct sdl_dropdown *dd, float mx, float my,
			      int win_w, int *selected_index);
void sdl_dropdown_draw_closed(SDL_Renderer *renderer,
			      const struct sdl_font *font,
			      const struct sdl_dropdown *dd);
void sdl_dropdown_draw_popup(SDL_Renderer *renderer,
			     const struct sdl_font *font,
			     const struct sdl_dropdown *dd, int win_w);

void sdl_ui_draw_tooltip(SDL_Renderer *renderer, const struct sdl_font *font,
			 const char *text, float mx, float my,
			 int win_w, int win_h);

struct sdl_log_entry {
	char text[SDL_LOG_ENTRY_MAX_LEN];
	int is_collision;
};

struct sdl_log_view {
	SDL_FRect rect;
	struct sdl_log_entry entries[SDL_LOG_MAX_ENTRIES];
	int count;
	int scroll_lines;
	int total_lines;
	int visible_lines;
	int auto_scroll;
	int dragging_thumb;
	float drag_grab_y;
};

void sdl_log_init(struct sdl_log_view *log);
void sdl_log_clear(struct sdl_log_view *log);
void sdl_log_append(struct sdl_log_view *log, const char *text,
		    int is_collision);
int sdl_log_handle_wheel(struct sdl_log_view *log, int wheel_y,
			 float mx, float my);
int sdl_log_handle_mouse_down(struct sdl_log_view *log, float mx, float my);
int sdl_log_handle_mouse_motion(struct sdl_log_view *log, float mx, float my);
int sdl_log_handle_mouse_up(struct sdl_log_view *log);
void sdl_log_draw(SDL_Renderer *renderer, const struct sdl_font *font,
		  struct sdl_log_view *log);

enum sdl_modal_result {
	SDL_MODAL_NONE = 0,
	SDL_MODAL_PRIMARY = 1,
	SDL_MODAL_SECONDARY = 2,
	SDL_MODAL_DISMISSED = 3,
};

struct sdl_modal {
	int active;
	int is_help;
	int dismiss_on_outside_click;
	char title[128];
	char primary[160];
	const char *body;
	char body_buf[512];
	int scroll_lines;
	int total_body_lines;
	int visible_body_lines;
	SDL_FRect panel_rect;
	SDL_FRect body_rect;
	struct sdl_button btn_primary;
	struct sdl_button btn_secondary;
	struct sdl_button btn_close_x;
};

void sdl_modal_init(struct sdl_modal *modal);
void sdl_modal_open_help(struct sdl_modal *modal, const char *title,
			 const char *instructions);
void sdl_modal_open_message(struct sdl_modal *modal, const char *title,
			    const char *primary, const char *secondary,
			    const char *primary_label,
			    const char *secondary_label);
void sdl_modal_close(struct sdl_modal *modal);
void sdl_modal_layout(struct sdl_modal *modal, int win_w, int win_h);
enum sdl_modal_result sdl_modal_handle_event(struct sdl_modal *modal,
					     const SDL_Event *ev,
					     int win_w, int win_h,
					     int *consumed,
					     int *needs_redraw);
void sdl_modal_draw(SDL_Renderer *renderer, const struct sdl_font *font,
		    struct sdl_modal *modal, int win_w, int win_h);

#endif /* DMOST_SDL_UI_H */
