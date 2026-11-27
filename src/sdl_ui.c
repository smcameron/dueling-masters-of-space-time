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
#include <stdio.h>
#include <string.h>

#include "sdl_gfx.h"
#include "sdl_ui.h"

#define LOG_LINE_SPACING 19.0f
#define LOG_PAD_X 8.0f
#define LOG_PAD_Y 5.0f
#define LOG_SCROLLBAR_W 10.0f
#define DD_ITEM_H 26.0f
#define MODAL_LINE_SPACING 20.0f

static SDL_Color col_rgba(Uint8 r, Uint8 g, Uint8 b, Uint8 a)
{
	SDL_Color c = { r, g, b, a };
	return c;
}

static void copy_str(char *dst, size_t dst_size, const char *src)
{
	if (dst_size == 0)
		return;
	if (src == NULL) {
		dst[0] = '\0';
		return;
	}
	snprintf(dst, dst_size, "%s", src);
}

static SDL_Rect frect_to_clip_rect(SDL_FRect r)
{
	SDL_Rect cr;

	cr.x = (int)r.x;
	cr.y = (int)r.y;
	cr.w = (int)r.w;
	cr.h = (int)r.h;
	if (cr.w < 0)
		cr.w = 0;
	if (cr.h < 0)
		cr.h = 0;
	return cr;
}

int sdl_point_in_rect(float x, float y, SDL_FRect r)
{
	return x >= r.x && x < (r.x + r.w) && y >= r.y && y < (r.y + r.h);
}

static void draw_triangle(SDL_Renderer *renderer, float x0, float y0,
			  float x1, float y1, float x2, float y2,
			  SDL_Color color)
{
	SDL_Vertex verts[3];
	int indices[3] = { 0, 1, 2 };

	verts[0].position.x = x0;
	verts[0].position.y = y0;
	verts[1].position.x = x1;
	verts[1].position.y = y1;
	verts[2].position.x = x2;
	verts[2].position.y = y2;
	for (int i = 0; i < 3; i++) {
		verts[i].color = color;
		verts[i].tex_coord.x = 0.0f;
		verts[i].tex_coord.y = 0.0f;
	}
	SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
	SDL_RenderGeometry(renderer, NULL, verts, 3, indices, 3);
}

/* ---------------- Buttons ---------------- */

void sdl_button_init(struct sdl_button *btn, const char *label,
		     const char *tooltip)
{
	if (btn == NULL)
		return;
	memset(btn, 0, sizeof(*btn));
	btn->label = label;
	btn->tooltip = tooltip;
	btn->visible = 1;
	btn->enabled = 1;
}

float sdl_button_preferred_width(const char *label)
{
	float w = sdl_font_measure_width(label ? label : "", SDL_UI_FONT_H);
	return w + 20.0f;
}

int sdl_button_handle_motion(struct sdl_button *btn, float mx, float my)
{
	int new_hover;

	if (btn == NULL || !btn->visible)
		return 0;
	new_hover = btn->enabled && sdl_point_in_rect(mx, my, btn->rect);
	if (new_hover != btn->hovered) {
		btn->hovered = new_hover;
		return 1;
	}
	return 0;
}

int sdl_button_handle_mouse_down(struct sdl_button *btn, float mx, float my)
{
	if (btn == NULL || !btn->visible || !btn->enabled)
		return 0;
	if (sdl_point_in_rect(mx, my, btn->rect)) {
		btn->pressed = 1;
		btn->hovered = 1;
		return 1;
	}
	return 0;
}

int sdl_button_handle_mouse_up(struct sdl_button *btn, float mx, float my,
			       int *clicked)
{
	int was_pressed;

	if (clicked != NULL)
		*clicked = 0;
	if (btn == NULL)
		return 0;
	was_pressed = btn->pressed;
	btn->pressed = 0;
	if (was_pressed && btn->visible && btn->enabled &&
	    sdl_point_in_rect(mx, my, btn->rect)) {
		if (clicked != NULL)
			*clicked = 1;
	}
	return was_pressed;
}

void sdl_button_draw(SDL_Renderer *renderer, const struct sdl_font *font,
		     const struct sdl_button *btn)
{
	SDL_Color fill, border, text_col;
	float cx, cy;

	if (renderer == NULL || font == NULL || btn == NULL || !btn->visible)
		return;

	if (!btn->enabled) {
		fill = col_rgba(28, 31, 38, 255);
		border = col_rgba(62, 68, 82, 255);
		text_col = col_rgba(115, 122, 136, 255);
	} else if (btn->accent) {
		if (btn->pressed)
			fill = col_rgba(34, 72, 118, 255);
		else if (btn->hovered)
			fill = col_rgba(62, 118, 182, 255);
		else
			fill = col_rgba(46, 96, 152, 255);
		border = col_rgba(120, 175, 235, 255);
		text_col = col_rgba(242, 246, 252, 255);
	} else {
		if (btn->pressed)
			fill = col_rgba(30, 35, 46, 255);
		else if (btn->hovered)
			fill = col_rgba(58, 68, 88, 255);
		else
			fill = col_rgba(42, 48, 62, 255);
		border = btn->hovered ? col_rgba(135, 155, 190, 255)
				      : col_rgba(88, 98, 120, 255);
		text_col = col_rgba(232, 238, 246, 255);
	}

	sdl_gfx_fill_rect(renderer, btn->rect.x, btn->rect.y,
			  btn->rect.w, btn->rect.h, fill);
	sdl_gfx_stroke_rect(renderer, btn->rect.x, btn->rect.y,
			    btn->rect.w, btn->rect.h, 1.0f, border);

	if (btn->label != NULL) {
		cx = btn->rect.x + btn->rect.w * 0.5f;
		cy = btn->rect.y + btn->rect.h * 0.5f;
		sdl_font_draw(renderer, font, cx, cy, SDL_UI_FONT_H,
			      btn->label, SDL_ALIGN_CENTER, text_col);
	}
}

/* ---------------- Dropdowns ---------------- */

void sdl_dropdown_init(struct sdl_dropdown *dd, const char *const *items,
		       int item_count, int active_index, const char *tooltip)
{
	if (dd == NULL)
		return;
	memset(dd, 0, sizeof(*dd));
	dd->items = items;
	dd->item_count = item_count;
	dd->active_index = active_index;
	dd->tooltip = tooltip;
	dd->enabled = 1;
	dd->hover_index = -1;
}

float sdl_dropdown_preferred_width(const char *const *items, int item_count)
{
	float max_w = 0.0f;
	int i;

	for (i = 0; i < item_count; i++) {
		float w = sdl_font_measure_width(items[i] ? items[i] : "",
						 SDL_UI_FONT_H);
		if (w > max_w)
			max_w = w;
	}
	return max_w + 34.0f;
}

SDL_FRect sdl_dropdown_popup_rect(const struct sdl_dropdown *dd, int win_w)
{
	SDL_FRect r = { 0.0f, 0.0f, 0.0f, 0.0f };
	float pw;

	if (dd == NULL || dd->item_count <= 0)
		return r;
	pw = sdl_dropdown_preferred_width(dd->items, dd->item_count);
	if (dd->rect.w > pw)
		pw = dd->rect.w;
	r.x = dd->rect.x;
	if (win_w > 0 && r.x + pw > (float)win_w - 4.0f)
		r.x = (float)win_w - pw - 4.0f;
	if (r.x < 4.0f)
		r.x = 4.0f;
	r.y = dd->rect.y + dd->rect.h + 1.0f;
	r.w = pw;
	r.h = (float)dd->item_count * DD_ITEM_H;
	return r;
}

int sdl_dropdown_handle_motion(struct sdl_dropdown *dd, float mx, float my,
			       int win_w)
{
	int changed = 0;
	int new_hover;
	int new_idx = -1;

	if (dd == NULL)
		return 0;

	new_hover = dd->enabled && sdl_point_in_rect(mx, my, dd->rect);
	if (new_hover != dd->hovered) {
		dd->hovered = new_hover;
		changed = 1;
	}

	if (dd->open) {
		SDL_FRect pop = sdl_dropdown_popup_rect(dd, win_w);
		if (sdl_point_in_rect(mx, my, pop)) {
			new_idx = (int)((my - pop.y) / DD_ITEM_H);
			if (new_idx < 0 || new_idx >= dd->item_count)
				new_idx = -1;
		}
	}
	if (new_idx != dd->hover_index) {
		dd->hover_index = new_idx;
		changed = 1;
	}
	return changed;
}

int sdl_dropdown_handle_click(struct sdl_dropdown *dd, float mx, float my,
			      int win_w, int *selected_index)
{
	if (selected_index != NULL)
		*selected_index = -1;
	if (dd == NULL || !dd->enabled)
		return 0;

	if (dd->open) {
		SDL_FRect pop = sdl_dropdown_popup_rect(dd, win_w);
		if (sdl_point_in_rect(mx, my, pop)) {
			int idx = (int)((my - pop.y) / DD_ITEM_H);
			dd->open = 0;
			dd->hover_index = -1;
			if (idx >= 0 && idx < dd->item_count) {
				dd->active_index = idx;
				if (selected_index != NULL)
					*selected_index = idx;
			}
			return 1;
		}
		dd->open = 0;
		dd->hover_index = -1;
		return 1;
	}

	if (sdl_point_in_rect(mx, my, dd->rect)) {
		dd->open = 1;
		dd->hover_index = dd->active_index;
		return 1;
	}
	return 0;
}

void sdl_dropdown_draw_closed(SDL_Renderer *renderer,
			      const struct sdl_font *font,
			      const struct sdl_dropdown *dd)
{
	SDL_Color fill, border, text_col, arrow_col;
	const char *label = "";
	float cy, ax, ay;

	if (renderer == NULL || font == NULL || dd == NULL)
		return;

	if (!dd->enabled) {
		fill = col_rgba(28, 31, 38, 255);
		border = col_rgba(62, 68, 82, 255);
		text_col = col_rgba(115, 122, 136, 255);
		arrow_col = text_col;
	} else {
		fill = (dd->hovered || dd->open) ? col_rgba(56, 66, 86, 255)
						 : col_rgba(40, 46, 60, 255);
		border = dd->open ? col_rgba(130, 175, 235, 255)
				  : (dd->hovered ? col_rgba(135, 155, 190, 255)
						 : col_rgba(88, 98, 120, 255));
		text_col = col_rgba(232, 238, 246, 255);
		arrow_col = col_rgba(185, 200, 225, 255);
	}

	sdl_gfx_fill_rect(renderer, dd->rect.x, dd->rect.y,
			  dd->rect.w, dd->rect.h, fill);
	sdl_gfx_stroke_rect(renderer, dd->rect.x, dd->rect.y,
			    dd->rect.w, dd->rect.h, 1.0f, border);

	if (dd->items != NULL && dd->active_index >= 0 &&
	    dd->active_index < dd->item_count &&
	    dd->items[dd->active_index] != NULL)
		label = dd->items[dd->active_index];

	cy = dd->rect.y + dd->rect.h * 0.5f;
	sdl_font_draw(renderer, font, dd->rect.x + 10.0f, cy, SDL_UI_FONT_H,
		      label, SDL_ALIGN_LEFT, text_col);

	ax = dd->rect.x + dd->rect.w - 13.0f;
	ay = cy;
	if (dd->open) {
		draw_triangle(renderer, ax - 4.0f, ay + 2.5f,
			      ax + 4.0f, ay + 2.5f, ax, ay - 2.5f, arrow_col);
	} else {
		draw_triangle(renderer, ax - 4.0f, ay - 2.5f,
			      ax + 4.0f, ay - 2.5f, ax, ay + 2.5f, arrow_col);
	}
}

void sdl_dropdown_draw_popup(SDL_Renderer *renderer,
			     const struct sdl_font *font,
			     const struct sdl_dropdown *dd, int win_w)
{
	SDL_FRect pop;
	int i;

	if (renderer == NULL || font == NULL || dd == NULL || !dd->open)
		return;

	pop = sdl_dropdown_popup_rect(dd, win_w);
	if (pop.w <= 0.0f || pop.h <= 0.0f)
		return;

	sdl_gfx_fill_rect(renderer, pop.x, pop.y, pop.w, pop.h,
			  col_rgba(32, 37, 48, 250));

	for (i = 0; i < dd->item_count; i++) {
		float iy = pop.y + (float)i * DD_ITEM_H;
		SDL_Color tc = col_rgba(225, 232, 242, 255);

		if (i == dd->hover_index) {
			sdl_gfx_fill_rect(renderer, pop.x + 1.0f, iy + 1.0f,
					  pop.w - 2.0f, DD_ITEM_H - 2.0f,
					  col_rgba(66, 98, 148, 255));
			tc = col_rgba(255, 255, 255, 255);
		}
		if (i == dd->active_index) {
			sdl_gfx_fill_rect(renderer, pop.x + 2.0f, iy + 4.0f,
					  3.0f, DD_ITEM_H - 8.0f,
					  col_rgba(110, 175, 245, 255));
		}
		if (dd->items[i] != NULL) {
			sdl_font_draw(renderer, font, pop.x + 12.0f,
				      iy + DD_ITEM_H * 0.5f, SDL_UI_FONT_H,
				      dd->items[i], SDL_ALIGN_LEFT, tc);
		}
	}

	sdl_gfx_stroke_rect(renderer, pop.x, pop.y, pop.w, pop.h, 1.0f,
			    col_rgba(125, 145, 180, 255));
}

/* ---------------- Tooltip ---------------- */

void sdl_ui_draw_tooltip(SDL_Renderer *renderer, const struct sdl_font *font,
			 const char *text, float mx, float my,
			 int win_w, int win_h)
{
	float tw, bw, bh, bx, by;

	if (renderer == NULL || font == NULL || text == NULL || *text == '\0')
		return;

	tw = sdl_font_measure_width(text, SDL_UI_FONT_H);
	bw = tw + 14.0f;
	bh = 24.0f;
	bx = mx + 12.0f;
	by = my + 20.0f;

	if (bx + bw > (float)win_w - 4.0f)
		bx = (float)win_w - bw - 4.0f;
	if (bx < 4.0f)
		bx = 4.0f;
	if (by + bh > (float)win_h - 4.0f)
		by = my - bh - 8.0f;
	if (by < 4.0f)
		by = 4.0f;

	sdl_gfx_fill_rect(renderer, bx, by, bw, bh, col_rgba(20, 24, 32, 240));
	sdl_gfx_stroke_rect(renderer, bx, by, bw, bh, 1.0f,
			    col_rgba(150, 165, 190, 255));
	sdl_font_draw(renderer, font, bx + 7.0f, by + bh * 0.5f,
		      SDL_UI_FONT_H, text, SDL_ALIGN_LEFT,
		      col_rgba(235, 240, 248, 255));
}

/* ---------------- Scrollable Log View ---------------- */

static void log_compute_metrics(const struct sdl_log_view *log,
				int *out_total, int *out_visible)
{
	float text_w;
	float inner_h;
	int total = 0;
	int visible;
	int i;

	text_w = log->rect.w - 2.0f * LOG_PAD_X - LOG_SCROLLBAR_W - 4.0f;
	if (text_w < 32.0f)
		text_w = 32.0f;
	inner_h = log->rect.h - 2.0f * LOG_PAD_Y;
	visible = (inner_h > 0.0f) ? (int)(inner_h / LOG_LINE_SPACING) : 1;
	if (visible < 1)
		visible = 1;

	for (i = 0; i < log->count; i++) {
		int lines = sdl_font_draw_wrapped(NULL, NULL, 0.0f, 0.0f,
						  text_w, SDL_UI_FONT_H,
						  LOG_LINE_SPACING,
						  log->entries[i].text,
						  col_rgba(255, 255, 255, 255),
						  0, 0);
		total += (lines > 0) ? lines : 1;
	}

	*out_total = total;
	*out_visible = visible;
}

static void log_clamp_scroll(struct sdl_log_view *log, int total, int visible)
{
	int max_scroll = total - visible;

	if (max_scroll < 0)
		max_scroll = 0;
	if (log->auto_scroll) {
		log->scroll_lines = max_scroll;
	} else {
		if (log->scroll_lines < 0)
			log->scroll_lines = 0;
		if (log->scroll_lines >= max_scroll) {
			log->scroll_lines = max_scroll;
			log->auto_scroll = 1;
		}
	}
	log->total_lines = total;
	log->visible_lines = visible;
}

static void log_scrollbar_rects(const struct sdl_log_view *log,
				SDL_FRect *track, SDL_FRect *thumb)
{
	float tr_x = log->rect.x + log->rect.w - LOG_SCROLLBAR_W - 2.0f;
	float tr_y = log->rect.y + 3.0f;
	float tr_h = log->rect.h - 6.0f;
	int max_scroll = log->total_lines - log->visible_lines;
	float thumb_h, ratio;

	track->x = tr_x;
	track->y = tr_y;
	track->w = LOG_SCROLLBAR_W;
	track->h = tr_h;

	if (max_scroll <= 0 || log->total_lines <= 0 || tr_h <= 0.0f) {
		*thumb = *track;
		return;
	}

	thumb_h = tr_h * ((float)log->visible_lines / (float)log->total_lines);
	if (thumb_h < 18.0f)
		thumb_h = 18.0f;
	if (thumb_h > tr_h)
		thumb_h = tr_h;

	ratio = (float)log->scroll_lines / (float)max_scroll;
	thumb->x = tr_x + 1.0f;
	thumb->y = tr_y + ratio * (tr_h - thumb_h);
	thumb->w = LOG_SCROLLBAR_W - 2.0f;
	thumb->h = thumb_h;
}

void sdl_log_init(struct sdl_log_view *log)
{
	if (log == NULL)
		return;
	memset(log, 0, sizeof(*log));
	log->auto_scroll = 1;
}

void sdl_log_clear(struct sdl_log_view *log)
{
	if (log == NULL)
		return;
	log->count = 0;
	log->scroll_lines = 0;
	log->total_lines = 0;
	log->auto_scroll = 1;
	log->dragging_thumb = 0;
}

void sdl_log_append(struct sdl_log_view *log, const char *text,
		    int is_collision)
{
	if (log == NULL || text == NULL)
		return;

	if (log->count >= SDL_LOG_MAX_ENTRIES) {
		memmove(&log->entries[0], &log->entries[1],
			sizeof(log->entries[0]) * (SDL_LOG_MAX_ENTRIES - 1));
		log->count = SDL_LOG_MAX_ENTRIES - 1;
	}
	copy_str(log->entries[log->count].text,
		 sizeof(log->entries[log->count].text), text);
	log->entries[log->count].is_collision = is_collision ? 1 : 0;
	log->count++;
	log->auto_scroll = 1;
}

int sdl_log_handle_wheel(struct sdl_log_view *log, int wheel_y,
			 float mx, float my)
{
	int total, visible, max_scroll, prev;

	if (log == NULL || wheel_y == 0 || !sdl_point_in_rect(mx, my, log->rect))
		return 0;

	log_compute_metrics(log, &total, &visible);
	log_clamp_scroll(log, total, visible);
	max_scroll = total - visible;
	if (max_scroll <= 0)
		return 1;

	prev = log->scroll_lines;
	log->auto_scroll = 0;
	log->scroll_lines -= wheel_y * 3;
	if (log->scroll_lines < 0)
		log->scroll_lines = 0;
	if (log->scroll_lines >= max_scroll) {
		log->scroll_lines = max_scroll;
		log->auto_scroll = 1;
	}
	return (log->scroll_lines != prev) ? 1 : 0;
}

int sdl_log_handle_mouse_down(struct sdl_log_view *log, float mx, float my)
{
	int total, visible, max_scroll;
	SDL_FRect track, thumb;

	if (log == NULL || !sdl_point_in_rect(mx, my, log->rect))
		return 0;

	log_compute_metrics(log, &total, &visible);
	log_clamp_scroll(log, total, visible);
	max_scroll = total - visible;
	if (max_scroll <= 0)
		return 0;

	log_scrollbar_rects(log, &track, &thumb);
	if (!sdl_point_in_rect(mx, my, track))
		return 0;

	if (sdl_point_in_rect(mx, my, thumb)) {
		log->dragging_thumb = 1;
		log->drag_grab_y = my - thumb.y;
		return 1;
	}

	/* Click on track above/below thumb jumps towards click position */
	{
		float travel = track.h - thumb.h;
		float target_y = my - thumb.h * 0.5f - track.y;
		float ratio = (travel > 0.0f) ? (target_y / travel) : 0.0f;

		if (ratio < 0.0f)
			ratio = 0.0f;
		if (ratio > 1.0f)
			ratio = 1.0f;
		log->auto_scroll = 0;
		log->scroll_lines = (int)(ratio * (float)max_scroll + 0.5f);
		if (log->scroll_lines >= max_scroll) {
			log->scroll_lines = max_scroll;
			log->auto_scroll = 1;
		}
		log->dragging_thumb = 1;
		log->drag_grab_y = thumb.h * 0.5f;
	}
	return 1;
}

int sdl_log_handle_mouse_motion(struct sdl_log_view *log, float mx, float my)
{
	SDL_FRect track, thumb;
	int max_scroll, prev;
	float travel, pos, ratio;

	(void)mx;
	if (log == NULL || !log->dragging_thumb)
		return 0;

	max_scroll = log->total_lines - log->visible_lines;
	if (max_scroll <= 0)
		return 0;

	log_scrollbar_rects(log, &track, &thumb);
	travel = track.h - thumb.h;
	if (travel <= 0.0f)
		return 0;

	pos = (my - log->drag_grab_y) - track.y;
	ratio = pos / travel;
	if (ratio < 0.0f)
		ratio = 0.0f;
	if (ratio > 1.0f)
		ratio = 1.0f;

	prev = log->scroll_lines;
	log->auto_scroll = 0;
	log->scroll_lines = (int)(ratio * (float)max_scroll + 0.5f);
	if (log->scroll_lines >= max_scroll) {
		log->scroll_lines = max_scroll;
		log->auto_scroll = 1;
	}
	return (log->scroll_lines != prev) ? 1 : 0;
}

int sdl_log_handle_mouse_up(struct sdl_log_view *log)
{
	if (log == NULL || !log->dragging_thumb)
		return 0;
	log->dragging_thumb = 0;
	return 1;
}

void sdl_log_draw(SDL_Renderer *renderer, const struct sdl_font *font,
		  struct sdl_log_view *log)
{
	int total, visible;
	float text_x, text_w, cur_y;
	int cur_line = 0;
	int i;
	SDL_Rect clip;

	if (renderer == NULL || font == NULL || log == NULL ||
	    log->rect.w <= 0.0f || log->rect.h <= 0.0f)
		return;

	sdl_gfx_fill_rect(renderer, log->rect.x, log->rect.y,
			  log->rect.w, log->rect.h, col_rgba(16, 19, 26, 255));
	sdl_gfx_stroke_rect(renderer, log->rect.x, log->rect.y,
			    log->rect.w, log->rect.h, 1.0f,
			    col_rgba(68, 78, 96, 255));

	log_compute_metrics(log, &total, &visible);
	log_clamp_scroll(log, total, visible);

	text_x = log->rect.x + LOG_PAD_X;
	text_w = log->rect.w - 2.0f * LOG_PAD_X - LOG_SCROLLBAR_W - 4.0f;
	if (text_w < 32.0f)
		text_w = 32.0f;
	cur_y = log->rect.y + LOG_PAD_Y;

	clip = frect_to_clip_rect(log->rect);
	clip.x += 1;
	clip.y += 1;
	clip.w -= 2;
	clip.h -= 2;
	SDL_RenderSetClipRect(renderer, &clip);

	for (i = 0; i < log->count; i++) {
		SDL_Color tc = log->entries[i].is_collision
			       ? col_rgba(242, 110, 56, 255)
			       : col_rgba(215, 222, 232, 255);
		int entry_lines = sdl_font_draw_wrapped(
			NULL, NULL, 0.0f, 0.0f, text_w, SDL_UI_FONT_H,
			LOG_LINE_SPACING, log->entries[i].text, tc, 0, 0);
		int entry_end;
		int local_off = 0;
		int remain_vis;

		if (entry_lines <= 0)
			entry_lines = 1;
		entry_end = cur_line + entry_lines;

		if (entry_end > log->scroll_lines &&
		    cur_line < log->scroll_lines + log->visible_lines) {
			if (log->scroll_lines > cur_line)
				local_off = log->scroll_lines - cur_line;
			remain_vis = (log->scroll_lines + log->visible_lines) -
				     (cur_line + local_off);
			if (remain_vis > 0) {
				int shown = entry_lines - local_off;
				if (shown > remain_vis)
					shown = remain_vis;
				sdl_font_draw_wrapped(
					renderer, font, text_x, cur_y, text_w,
					SDL_UI_FONT_H, LOG_LINE_SPACING,
					log->entries[i].text, tc,
					local_off, remain_vis);
				cur_y += (float)shown * LOG_LINE_SPACING;
			}
		}
		cur_line = entry_end;
	}

	SDL_RenderSetClipRect(renderer, NULL);

	if (log->total_lines > log->visible_lines) {
		SDL_FRect track, thumb;

		log_scrollbar_rects(log, &track, &thumb);
		sdl_gfx_fill_rect(renderer, track.x, track.y, track.w, track.h,
				  col_rgba(26, 30, 40, 255));
		sdl_gfx_fill_rect(renderer, thumb.x, thumb.y, thumb.w, thumb.h,
				  log->dragging_thumb
				  ? col_rgba(125, 155, 200, 255)
				  : col_rgba(88, 104, 134, 255));
	}
}

/* ---------------- Modal Dialogs ---------------- */

void sdl_modal_init(struct sdl_modal *modal)
{
	if (modal == NULL)
		return;
	memset(modal, 0, sizeof(*modal));
}

void sdl_modal_open_help(struct sdl_modal *modal, const char *title,
			 const char *instructions)
{
	if (modal == NULL)
		return;
	memset(modal, 0, sizeof(*modal));
	modal->active = 1;
	modal->is_help = 1;
	modal->dismiss_on_outside_click = 1;
	copy_str(modal->title, sizeof(modal->title), title ? title : "Help");
	modal->body = instructions ? instructions : "";
	sdl_button_init(&modal->btn_primary, "Close", NULL);
	modal->btn_primary.accent = 1;
	sdl_button_init(&modal->btn_secondary, "", NULL);
	modal->btn_secondary.visible = 0;
	sdl_button_init(&modal->btn_close_x, "X", NULL);
}

void sdl_modal_open_message(struct sdl_modal *modal, const char *title,
			    const char *primary, const char *secondary,
			    const char *primary_label,
			    const char *secondary_label)
{
	if (modal == NULL)
		return;
	memset(modal, 0, sizeof(*modal));
	modal->active = 1;
	modal->is_help = 0;
	modal->dismiss_on_outside_click = 0;
	copy_str(modal->title, sizeof(modal->title), title ? title : "");
	copy_str(modal->primary, sizeof(modal->primary), primary ? primary : "");
	copy_str(modal->body_buf, sizeof(modal->body_buf),
		 secondary ? secondary : "");
	modal->body = modal->body_buf;

	sdl_button_init(&modal->btn_primary,
			primary_label ? primary_label : "OK", NULL);
	modal->btn_primary.accent = 1;

	sdl_button_init(&modal->btn_secondary,
			secondary_label ? secondary_label : "", NULL);
	modal->btn_secondary.visible = (secondary_label != NULL &&
					*secondary_label != '\0');
	sdl_button_init(&modal->btn_close_x, "X", NULL);
}

void sdl_modal_close(struct sdl_modal *modal)
{
	if (modal == NULL)
		return;
	modal->active = 0;
}

void sdl_modal_layout(struct sdl_modal *modal, int win_w, int win_h)
{
	float pw, ph, px, py;
	float title_h = 34.0f;
	float footer_h = 46.0f;
	float pad = 16.0f;
	float btn_h = 30.0f;
	float w1, w2, bx, by;

	if (modal == NULL || !modal->active)
		return;

	if (modal->is_help) {
		pw = (float)(win_w - 40);
		ph = (float)(win_h - 40);
		if (pw > 720.0f)
			pw = 720.0f;
		if (ph > 540.0f)
			ph = 540.0f;
	} else {
		pw = (float)(win_w - 40);
		ph = (float)(win_h - 40);
		if (pw > 540.0f)
			pw = 540.0f;
		if (ph > 240.0f)
			ph = 240.0f;
	}
	if (pw < 260.0f)
		pw = 260.0f;
	if (ph < 160.0f)
		ph = 160.0f;

	px = ((float)win_w - pw) * 0.5f;
	py = ((float)win_h - ph) * 0.5f;
	modal->panel_rect = (SDL_FRect){ px, py, pw, ph };

	modal->btn_close_x.rect = (SDL_FRect){
		px + pw - 30.0f, py + 5.0f, 24.0f, 24.0f
	};

	modal->body_rect = (SDL_FRect){
		px + pad,
		py + title_h + 10.0f,
		pw - 2.0f * pad,
		ph - title_h - footer_h - 14.0f
	};
	if (modal->body_rect.h < 20.0f)
		modal->body_rect.h = 20.0f;

	by = py + ph - footer_h + 8.0f;
	w1 = sdl_button_preferred_width(modal->btn_primary.label);
	if (w1 < 88.0f)
		w1 = 88.0f;
	bx = px + pw - pad - w1;
	modal->btn_primary.rect = (SDL_FRect){ bx, by, w1, btn_h };

	if (modal->btn_secondary.visible) {
		w2 = sdl_button_preferred_width(modal->btn_secondary.label);
		if (w2 < 88.0f)
			w2 = 88.0f;
		modal->btn_secondary.rect = (SDL_FRect){
			bx - 10.0f - w2, by, w2, btn_h
		};
	}

	{
		float text_y = modal->body_rect.y;
		float avail_h = modal->body_rect.h;
		float text_w = modal->body_rect.w - 14.0f;

		if (!modal->is_help && modal->primary[0] != '\0') {
			text_y += 30.0f;
			avail_h -= 30.0f;
		}
		if (text_w < 32.0f)
			text_w = 32.0f;
		modal->visible_body_lines = (avail_h > 0.0f)
			? (int)(avail_h / MODAL_LINE_SPACING) : 1;
		if (modal->visible_body_lines < 1)
			modal->visible_body_lines = 1;
		modal->total_body_lines = sdl_font_draw_wrapped(
			NULL, NULL, 0.0f, 0.0f, text_w, SDL_UI_FONT_H,
			MODAL_LINE_SPACING, modal->body,
			col_rgba(255, 255, 255, 255), 0, 0);
		{
			int max_scroll = modal->total_body_lines -
					 modal->visible_body_lines;
			if (max_scroll < 0)
				max_scroll = 0;
			if (modal->scroll_lines < 0)
				modal->scroll_lines = 0;
			if (modal->scroll_lines > max_scroll)
				modal->scroll_lines = max_scroll;
		}
	}
}

static void modal_scroll_by(struct sdl_modal *modal, int delta,
			    int *needs_redraw)
{
	int max_scroll = modal->total_body_lines - modal->visible_body_lines;
	int prev = modal->scroll_lines;

	if (max_scroll < 0)
		max_scroll = 0;
	modal->scroll_lines += delta;
	if (modal->scroll_lines < 0)
		modal->scroll_lines = 0;
	if (modal->scroll_lines > max_scroll)
		modal->scroll_lines = max_scroll;
	if (modal->scroll_lines != prev && needs_redraw != NULL)
		*needs_redraw = 1;
}

enum sdl_modal_result sdl_modal_handle_event(struct sdl_modal *modal,
					     const SDL_Event *ev,
					     int win_w, int win_h,
					     int *consumed,
					     int *needs_redraw)
{
	if (consumed != NULL)
		*consumed = 0;
	if (needs_redraw != NULL)
		*needs_redraw = 0;
	if (modal == NULL || !modal->active || ev == NULL)
		return SDL_MODAL_NONE;

	sdl_modal_layout(modal, win_w, win_h);

	switch (ev->type) {
	case SDL_MOUSEMOTION: {
		float mx = (float)ev->motion.x;
		float my = (float)ev->motion.y;
		int ch = 0;

		ch |= sdl_button_handle_motion(&modal->btn_primary, mx, my);
		ch |= sdl_button_handle_motion(&modal->btn_secondary, mx, my);
		ch |= sdl_button_handle_motion(&modal->btn_close_x, mx, my);
		if (consumed != NULL)
			*consumed = 1;
		if (needs_redraw != NULL)
			*needs_redraw = ch;
		return SDL_MODAL_NONE;
	}
	case SDL_MOUSEBUTTONDOWN:
		if (consumed != NULL)
			*consumed = 1;
		if (ev->button.button == SDL_BUTTON_LEFT) {
			float mx = (float)ev->button.x;
			float my = (float)ev->button.y;

			if (!sdl_point_in_rect(mx, my, modal->panel_rect)) {
				if (modal->dismiss_on_outside_click) {
					modal->active = 0;
					if (needs_redraw != NULL)
						*needs_redraw = 1;
					return SDL_MODAL_DISMISSED;
				}
				return SDL_MODAL_NONE;
			}
			if (sdl_button_handle_mouse_down(&modal->btn_primary, mx, my) ||
			    sdl_button_handle_mouse_down(&modal->btn_secondary, mx, my) ||
			    sdl_button_handle_mouse_down(&modal->btn_close_x, mx, my)) {
				if (needs_redraw != NULL)
					*needs_redraw = 1;
			}
		}
		return SDL_MODAL_NONE;
	case SDL_MOUSEBUTTONUP:
		if (consumed != NULL)
			*consumed = 1;
		if (ev->button.button == SDL_BUTTON_LEFT) {
			float mx = (float)ev->button.x;
			float my = (float)ev->button.y;
			int c1 = 0, c2 = 0, cx = 0;
			int handled = 0;

			handled |= sdl_button_handle_mouse_up(&modal->btn_primary,
							      mx, my, &c1);
			handled |= sdl_button_handle_mouse_up(&modal->btn_secondary,
							      mx, my, &c2);
			handled |= sdl_button_handle_mouse_up(&modal->btn_close_x,
							      mx, my, &cx);
			if (handled && needs_redraw != NULL)
				*needs_redraw = 1;
			if (c1) {
				modal->active = 0;
				if (needs_redraw != NULL)
					*needs_redraw = 1;
				return SDL_MODAL_PRIMARY;
			}
			if (c2) {
				modal->active = 0;
				if (needs_redraw != NULL)
					*needs_redraw = 1;
				return SDL_MODAL_SECONDARY;
			}
			if (cx) {
				modal->active = 0;
				if (needs_redraw != NULL)
					*needs_redraw = 1;
				return SDL_MODAL_DISMISSED;
			}
		}
		return SDL_MODAL_NONE;
	case SDL_MOUSEWHEEL:
		if (consumed != NULL)
			*consumed = 1;
		if (ev->wheel.y != 0)
			modal_scroll_by(modal, -ev->wheel.y * 3, needs_redraw);
		return SDL_MODAL_NONE;
	case SDL_KEYDOWN:
		if (consumed != NULL)
			*consumed = 1;
		switch (ev->key.keysym.sym) {
		case SDLK_ESCAPE:
			modal->active = 0;
			if (needs_redraw != NULL)
				*needs_redraw = 1;
			return SDL_MODAL_DISMISSED;
		case SDLK_RETURN:
		case SDLK_KP_ENTER:
			modal->active = 0;
			if (needs_redraw != NULL)
				*needs_redraw = 1;
			return SDL_MODAL_PRIMARY;
		case SDLK_UP:
			modal_scroll_by(modal, -1, needs_redraw);
			break;
		case SDLK_DOWN:
			modal_scroll_by(modal, 1, needs_redraw);
			break;
		case SDLK_PAGEUP:
			modal_scroll_by(modal, -modal->visible_body_lines,
					needs_redraw);
			break;
		case SDLK_PAGEDOWN:
			modal_scroll_by(modal, modal->visible_body_lines,
					needs_redraw);
			break;
		default:
			break;
		}
		return SDL_MODAL_NONE;
	default:
		return SDL_MODAL_NONE;
	}
}

void sdl_modal_draw(SDL_Renderer *renderer, const struct sdl_font *font,
		    struct sdl_modal *modal, int win_w, int win_h)
{
	SDL_FRect p;
	float title_h = 34.0f;
	float text_y;
	SDL_Rect clip;

	if (renderer == NULL || font == NULL || modal == NULL || !modal->active)
		return;

	sdl_modal_layout(modal, win_w, win_h);
	p = modal->panel_rect;

	/* Backdrop dim */
	sdl_gfx_fill_rect(renderer, 0.0f, 0.0f, (float)win_w, (float)win_h,
			  col_rgba(0, 0, 0, 155));

	/* Panel background & title bar */
	sdl_gfx_fill_rect(renderer, p.x, p.y, p.w, p.h,
			  col_rgba(24, 28, 38, 252));
	sdl_gfx_fill_rect(renderer, p.x, p.y, p.w, title_h,
			  col_rgba(36, 44, 60, 255));
	sdl_gfx_fill_rect(renderer, p.x, p.y + title_h, p.w, 1.0f,
			  col_rgba(88, 102, 130, 255));

	sdl_font_draw(renderer, font, p.x + 14.0f, p.y + title_h * 0.5f,
		      SDL_UI_FONT_H, modal->title, SDL_ALIGN_LEFT,
		      col_rgba(240, 245, 255, 255));

	text_y = modal->body_rect.y;
	if (!modal->is_help && modal->primary[0] != '\0') {
		sdl_font_draw(renderer, font, modal->body_rect.x,
			      text_y + 10.0f, 20.0f, modal->primary,
			      SDL_ALIGN_LEFT, col_rgba(255, 242, 185, 255));
		text_y += 30.0f;
	}

	{
		SDL_FRect body_clip = modal->body_rect;
		float text_w = modal->body_rect.w - 14.0f;

		body_clip.y = text_y;
		body_clip.h = (modal->body_rect.y + modal->body_rect.h) - text_y;
		if (body_clip.h < 0.0f)
			body_clip.h = 0.0f;
		if (text_w < 32.0f)
			text_w = 32.0f;

		clip = frect_to_clip_rect(body_clip);
		SDL_RenderSetClipRect(renderer, &clip);
		sdl_font_draw_wrapped(renderer, font, modal->body_rect.x,
				      text_y, text_w, SDL_UI_FONT_H,
				      MODAL_LINE_SPACING, modal->body,
				      col_rgba(220, 226, 236, 255),
				      modal->scroll_lines,
				      modal->visible_body_lines);
		SDL_RenderSetClipRect(renderer, NULL);

		if (modal->total_body_lines > modal->visible_body_lines &&
		    body_clip.h > 8.0f) {
			int max_scroll = modal->total_body_lines -
					 modal->visible_body_lines;
			float tr_x = modal->body_rect.x + modal->body_rect.w - 8.0f;
			float tr_y = body_clip.y;
			float tr_h = body_clip.h;
			float thumb_h = tr_h * ((float)modal->visible_body_lines /
						(float)modal->total_body_lines);
			float ratio = (max_scroll > 0)
				      ? ((float)modal->scroll_lines /
					 (float)max_scroll)
				      : 0.0f;
			float thumb_y;

			if (thumb_h < 18.0f)
				thumb_h = 18.0f;
			if (thumb_h > tr_h)
				thumb_h = tr_h;
			thumb_y = tr_y + ratio * (tr_h - thumb_h);

			sdl_gfx_fill_rect(renderer, tr_x, tr_y, 8.0f, tr_h,
					  col_rgba(30, 35, 46, 255));
			sdl_gfx_fill_rect(renderer, tr_x + 1.0f, thumb_y,
					  6.0f, thumb_h,
					  col_rgba(95, 114, 148, 255));
		}
	}

	sdl_button_draw(renderer, font, &modal->btn_close_x);
	sdl_button_draw(renderer, font, &modal->btn_secondary);
	sdl_button_draw(renderer, font, &modal->btn_primary);

	sdl_gfx_stroke_rect(renderer, p.x, p.y, p.w, p.h, 1.5f,
			    col_rgba(120, 140, 175, 255));
}
