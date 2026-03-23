/*
 * Copyright (C) 2024 TeraTerm Project
 * All rights reserved.
 *
 * macOS native window management using Cocoa/AppKit.
 * Provides the windowing backend for Tera Term on macOS.
 */
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/* Opaque handle for macOS native window */
typedef void* TTMacWindow;
typedef void* TTMacView;
typedef void* TTMacMenu;
typedef void* TTMacTimer;
typedef void* TTMacFont;
typedef void* TTMacGraphicsContext;

/* Window creation/destruction */
TTMacWindow tt_mac_window_create(int x, int y, int width, int height, const char* title);
void tt_mac_window_destroy(TTMacWindow window);
void tt_mac_window_show(TTMacWindow window);
void tt_mac_window_hide(TTMacWindow window);
void tt_mac_window_set_title(TTMacWindow window, const char* title);
void tt_mac_window_set_title_w(TTMacWindow window, const wchar_t* title);
void tt_mac_window_get_rect(TTMacWindow window, int* x, int* y, int* w, int* h);
void tt_mac_window_set_rect(TTMacWindow window, int x, int y, int w, int h);
void tt_mac_window_invalidate(TTMacWindow window);
void tt_mac_window_set_topmost(TTMacWindow window, int topmost);

/* Terminal view */
TTMacView tt_mac_termview_create(TTMacWindow window);
void tt_mac_termview_set_font(TTMacView view, const char* family, int size, int bold, int italic);
void tt_mac_termview_set_colors(TTMacView view, unsigned int fg, unsigned int bg);
void tt_mac_termview_invalidate(TTMacView view);
void tt_mac_termview_scroll(TTMacView view, int lines);

/* Drawing context */
TTMacGraphicsContext tt_mac_gc_begin(TTMacView view);
void tt_mac_gc_end(TTMacGraphicsContext gc);
void tt_mac_gc_set_color(TTMacGraphicsContext gc, unsigned int color);
void tt_mac_gc_set_bg_color(TTMacGraphicsContext gc, unsigned int color);
void tt_mac_gc_draw_text(TTMacGraphicsContext gc, int x, int y, const char* text, int len);
void tt_mac_gc_draw_text_w(TTMacGraphicsContext gc, int x, int y, const wchar_t* text, int len);
void tt_mac_gc_fill_rect(TTMacGraphicsContext gc, int x, int y, int w, int h);
void tt_mac_gc_draw_line(TTMacGraphicsContext gc, int x1, int y1, int x2, int y2);

/* Font metrics */
TTMacFont tt_mac_font_create(const char* family, int size, int bold, int italic);
void tt_mac_font_destroy(TTMacFont font);
void tt_mac_font_get_metrics(TTMacFont font, int* char_width, int* char_height, int* ascent);

/* Menu bar */
TTMacMenu tt_mac_menu_create(void);
void tt_mac_menu_add_item(TTMacMenu menu, const char* title, int command_id, const char* shortcut);
TTMacMenu tt_mac_menu_add_submenu(TTMacMenu parent, const char* title);
void tt_mac_menu_add_separator(TTMacMenu menu);
void tt_mac_menubar_set(TTMacMenu menu);

/* Timer */
TTMacTimer tt_mac_timer_create(double interval_seconds, void (*callback)(void* ctx), void* ctx);
void tt_mac_timer_destroy(TTMacTimer timer);

/* Clipboard */
int tt_mac_clipboard_set_text(const char* text);
int tt_mac_clipboard_set_text_w(const wchar_t* text);
char* tt_mac_clipboard_get_text(void);
wchar_t* tt_mac_clipboard_get_text_w(void);
void tt_mac_clipboard_free(void* ptr);

/* Application event loop */
void tt_mac_app_init(void);
void tt_mac_app_run(void);
void tt_mac_app_quit(void);

/* Message box / dialog */
int tt_mac_messagebox(TTMacWindow parent, const char* text, const char* caption, unsigned int type);

/* File dialogs */
char* tt_mac_open_file_dialog(TTMacWindow parent, const char* filter, const char* initial_dir);
char* tt_mac_save_file_dialog(TTMacWindow parent, const char* filter, const char* initial_dir);

#ifdef __cplusplus
}
#endif
