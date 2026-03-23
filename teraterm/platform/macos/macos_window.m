/*
 * Copyright (C) 2024 TeraTerm Project
 * All rights reserved.
 *
 * macOS native window implementation using Cocoa/AppKit.
 */

#import <Cocoa/Cocoa.h>
#include "macos_window.h"
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

/* --- Helper: wchar_t to NSString --- */
static NSString* nsstring_from_wchar(const wchar_t* wstr) {
    if (!wstr) return nil;
    size_t len = wcslen(wstr);
    /* wchar_t is 4 bytes on macOS (UTF-32) */
    return [[NSString alloc] initWithBytes:wstr
                                    length:len * sizeof(wchar_t)
                                  encoding:NSUTF32LittleEndianStringEncoding];
}

/* --- Terminal View (custom NSView) --- */
@interface TTTerminalView : NSView
@property (nonatomic, strong) NSFont *termFont;
@property (nonatomic, strong) NSColor *fgColor;
@property (nonatomic, strong) NSColor *bgColor;
@end

@implementation TTTerminalView

- (instancetype)initWithFrame:(NSRect)frame {
    self = [super initWithFrame:frame];
    if (self) {
        _termFont = [NSFont fontWithName:@"Menlo" size:14.0];
        _fgColor = [NSColor whiteColor];
        _bgColor = [NSColor blackColor];
    }
    return self;
}

- (BOOL)isFlipped {
    return YES;  /* Top-left origin like Windows */
}

- (BOOL)acceptsFirstResponder {
    return YES;
}

- (void)drawRect:(NSRect)dirtyRect {
    [self.bgColor setFill];
    NSRectFill(dirtyRect);
}

- (void)keyDown:(NSEvent *)event {
    /* TODO: Forward to terminal input handler */
    [self interpretKeyEvents:@[event]];
}

@end

/* --- Window Delegate --- */
@interface TTWindowDelegate : NSObject <NSWindowDelegate>
@end

@implementation TTWindowDelegate

- (BOOL)windowShouldClose:(id)sender {
    /* TODO: Confirm close if session active */
    return YES;
}

- (void)windowWillClose:(NSNotification *)notification {
    /* TODO: Clean up terminal session */
}

@end

/* --- Window Management --- */

TTMacWindow tt_mac_window_create(int x, int y, int width, int height, const char* title) {
    @autoreleasepool {
        NSRect frame = NSMakeRect(x, y, width, height);
        NSWindowStyleMask style = NSWindowStyleMaskTitled |
                                  NSWindowStyleMaskClosable |
                                  NSWindowStyleMaskMiniaturizable |
                                  NSWindowStyleMaskResizable;

        NSWindow* window = [[NSWindow alloc] initWithContentRect:frame
                                                       styleMask:style
                                                         backing:NSBackingStoreBuffered
                                                           defer:NO];

        TTWindowDelegate* delegate = [[TTWindowDelegate alloc] init];
        window.delegate = delegate;

        if (title) {
            window.title = [NSString stringWithUTF8String:title];
        }

        window.minSize = NSMakeSize(320, 200);
        window.backgroundColor = [NSColor blackColor];

        return (__bridge_retained void*)window;
    }
}

void tt_mac_window_destroy(TTMacWindow win) {
    if (!win) return;
    @autoreleasepool {
        NSWindow* window = (__bridge_transfer NSWindow*)win;
        [window close];
    }
}

void tt_mac_window_show(TTMacWindow win) {
    if (!win) return;
    @autoreleasepool {
        NSWindow* window = (__bridge NSWindow*)win;
        [window makeKeyAndOrderFront:nil];
    }
}

void tt_mac_window_hide(TTMacWindow win) {
    if (!win) return;
    @autoreleasepool {
        NSWindow* window = (__bridge NSWindow*)win;
        [window orderOut:nil];
    }
}

void tt_mac_window_set_title(TTMacWindow win, const char* title) {
    if (!win || !title) return;
    @autoreleasepool {
        NSWindow* window = (__bridge NSWindow*)win;
        window.title = [NSString stringWithUTF8String:title];
    }
}

void tt_mac_window_set_title_w(TTMacWindow win, const wchar_t* title) {
    if (!win || !title) return;
    @autoreleasepool {
        NSWindow* window = (__bridge NSWindow*)win;
        NSString* str = nsstring_from_wchar(title);
        if (str) window.title = str;
    }
}

void tt_mac_window_get_rect(TTMacWindow win, int* x, int* y, int* w, int* h) {
    if (!win) return;
    @autoreleasepool {
        NSWindow* window = (__bridge NSWindow*)win;
        NSRect frame = window.frame;
        if (x) *x = (int)frame.origin.x;
        if (y) *y = (int)frame.origin.y;
        if (w) *w = (int)frame.size.width;
        if (h) *h = (int)frame.size.height;
    }
}

void tt_mac_window_set_rect(TTMacWindow win, int x, int y, int w, int h) {
    if (!win) return;
    @autoreleasepool {
        NSWindow* window = (__bridge NSWindow*)win;
        NSRect frame = NSMakeRect(x, y, w, h);
        [window setFrame:frame display:YES animate:NO];
    }
}

void tt_mac_window_invalidate(TTMacWindow win) {
    if (!win) return;
    @autoreleasepool {
        NSWindow* window = (__bridge NSWindow*)win;
        [window.contentView setNeedsDisplay:YES];
    }
}

void tt_mac_window_set_topmost(TTMacWindow win, int topmost) {
    if (!win) return;
    @autoreleasepool {
        NSWindow* window = (__bridge NSWindow*)win;
        window.level = topmost ? NSFloatingWindowLevel : NSNormalWindowLevel;
    }
}

/* --- Terminal View --- */

TTMacView tt_mac_termview_create(TTMacWindow win) {
    if (!win) return NULL;
    @autoreleasepool {
        NSWindow* window = (__bridge NSWindow*)win;
        TTTerminalView* view = [[TTTerminalView alloc] initWithFrame:window.contentView.bounds];
        view.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
        [window.contentView addSubview:view];
        return (__bridge_retained void*)view;
    }
}

void tt_mac_termview_set_font(TTMacView v, const char* family, int size, int bold, int italic) {
    if (!v) return;
    @autoreleasepool {
        TTTerminalView* view = (__bridge TTTerminalView*)v;
        NSString* fontName = family ? [NSString stringWithUTF8String:family] : @"Menlo";
        NSFont* font = [NSFont fontWithName:fontName size:(CGFloat)size];
        if (!font) {
            font = [NSFont monospacedSystemFontOfSize:(CGFloat)size weight:bold ? NSFontWeightBold : NSFontWeightRegular];
        }
        if (bold && font) {
            NSFont* boldFont = [[NSFontManager sharedFontManager] convertFont:font toHaveTrait:NSBoldFontMask];
            if (boldFont) font = boldFont;
        }
        if (italic && font) {
            NSFont* italicFont = [[NSFontManager sharedFontManager] convertFont:font toHaveTrait:NSItalicFontMask];
            if (italicFont) font = italicFont;
        }
        view.termFont = font;
        [view setNeedsDisplay:YES];
    }
}

void tt_mac_termview_set_colors(TTMacView v, unsigned int fg, unsigned int bg) {
    if (!v) return;
    @autoreleasepool {
        TTTerminalView* view = (__bridge TTTerminalView*)v;
        view.fgColor = [NSColor colorWithRed:((fg >> 0) & 0xFF) / 255.0
                                       green:((fg >> 8) & 0xFF) / 255.0
                                        blue:((fg >> 16) & 0xFF) / 255.0
                                       alpha:1.0];
        view.bgColor = [NSColor colorWithRed:((bg >> 0) & 0xFF) / 255.0
                                       green:((bg >> 8) & 0xFF) / 255.0
                                        blue:((bg >> 16) & 0xFF) / 255.0
                                       alpha:1.0];
        [view setNeedsDisplay:YES];
    }
}

void tt_mac_termview_invalidate(TTMacView v) {
    if (!v) return;
    @autoreleasepool {
        TTTerminalView* view = (__bridge TTTerminalView*)v;
        [view setNeedsDisplay:YES];
    }
}

void tt_mac_termview_scroll(TTMacView v, int lines) {
    if (!v) return;
    @autoreleasepool {
        TTTerminalView* view = (__bridge TTTerminalView*)v;
        /* TODO: implement scrollback */
        [view setNeedsDisplay:YES];
    }
}

/* --- Drawing Context --- */

TTMacGraphicsContext tt_mac_gc_begin(TTMacView v) {
    if (!v) return NULL;
    NSGraphicsContext* ctx = [NSGraphicsContext currentContext];
    return (__bridge void*)ctx;
}

void tt_mac_gc_end(TTMacGraphicsContext gc) {
    /* Nothing to do - context managed by Cocoa draw cycle */
}

void tt_mac_gc_set_color(TTMacGraphicsContext gc, unsigned int color) {
    @autoreleasepool {
        NSColor* c = [NSColor colorWithRed:((color >> 0) & 0xFF) / 255.0
                                     green:((color >> 8) & 0xFF) / 255.0
                                      blue:((color >> 16) & 0xFF) / 255.0
                                     alpha:1.0];
        [c setFill];
        [c setStroke];
    }
}

void tt_mac_gc_set_bg_color(TTMacGraphicsContext gc, unsigned int color) {
    @autoreleasepool {
        NSColor* c = [NSColor colorWithRed:((color >> 0) & 0xFF) / 255.0
                                     green:((color >> 8) & 0xFF) / 255.0
                                      blue:((color >> 16) & 0xFF) / 255.0
                                     alpha:1.0];
        [c setFill];
    }
}

void tt_mac_gc_draw_text(TTMacGraphicsContext gc, int x, int y, const char* text, int len) {
    if (!text) return;
    @autoreleasepool {
        NSString* str = [[NSString alloc] initWithBytes:text length:len encoding:NSUTF8StringEncoding];
        if (!str) return;
        NSDictionary* attrs = @{
            NSFontAttributeName: [NSFont monospacedSystemFontOfSize:14.0 weight:NSFontWeightRegular],
            NSForegroundColorAttributeName: [NSColor whiteColor]
        };
        [str drawAtPoint:NSMakePoint(x, y) withAttributes:attrs];
    }
}

void tt_mac_gc_draw_text_w(TTMacGraphicsContext gc, int x, int y, const wchar_t* text, int len) {
    if (!text) return;
    @autoreleasepool {
        NSString* str = [[NSString alloc] initWithBytes:text
                                                 length:len * sizeof(wchar_t)
                                               encoding:NSUTF32LittleEndianStringEncoding];
        if (!str) return;
        NSDictionary* attrs = @{
            NSFontAttributeName: [NSFont monospacedSystemFontOfSize:14.0 weight:NSFontWeightRegular],
            NSForegroundColorAttributeName: [NSColor whiteColor]
        };
        [str drawAtPoint:NSMakePoint(x, y) withAttributes:attrs];
    }
}

void tt_mac_gc_fill_rect(TTMacGraphicsContext gc, int x, int y, int w, int h) {
    NSRectFill(NSMakeRect(x, y, w, h));
}

void tt_mac_gc_draw_line(TTMacGraphicsContext gc, int x1, int y1, int x2, int y2) {
    NSBezierPath* path = [NSBezierPath bezierPath];
    [path moveToPoint:NSMakePoint(x1, y1)];
    [path lineToPoint:NSMakePoint(x2, y2)];
    [path stroke];
}

/* --- Font --- */

TTMacFont tt_mac_font_create(const char* family, int size, int bold, int italic) {
    @autoreleasepool {
        NSString* fontName = family ? [NSString stringWithUTF8String:family] : @"Menlo";
        NSFont* font = [NSFont fontWithName:fontName size:(CGFloat)size];
        if (!font) {
            font = [NSFont monospacedSystemFontOfSize:(CGFloat)size weight:NSFontWeightRegular];
        }
        if (bold) {
            NSFont* bf = [[NSFontManager sharedFontManager] convertFont:font toHaveTrait:NSBoldFontMask];
            if (bf) font = bf;
        }
        if (italic) {
            NSFont* itf = [[NSFontManager sharedFontManager] convertFont:font toHaveTrait:NSItalicFontMask];
            if (itf) font = itf;
        }
        return (__bridge_retained void*)font;
    }
}

void tt_mac_font_destroy(TTMacFont f) {
    if (!f) return;
    @autoreleasepool {
        NSFont* font __attribute__((unused)) = (__bridge_transfer NSFont*)f;
    }
}

void tt_mac_font_get_metrics(TTMacFont f, int* char_width, int* char_height, int* ascent) {
    if (!f) return;
    @autoreleasepool {
        NSFont* font = (__bridge NSFont*)f;
        NSDictionary* attrs = @{NSFontAttributeName: font};
        NSSize charSize = [@"W" sizeWithAttributes:attrs];
        if (char_width)  *char_width  = (int)ceil(charSize.width);
        if (char_height) *char_height = (int)ceil(font.ascender - font.descender + font.leading);
        if (ascent)      *ascent      = (int)ceil(font.ascender);
    }
}

/* --- Menu --- */

TTMacMenu tt_mac_menu_create(void) {
    @autoreleasepool {
        NSMenu* menu = [[NSMenu alloc] init];
        return (__bridge_retained void*)menu;
    }
}

void tt_mac_menu_add_item(TTMacMenu m, const char* title, int command_id, const char* shortcut) {
    if (!m) return;
    @autoreleasepool {
        NSMenu* menu = (__bridge NSMenu*)m;
        NSString* t = title ? [NSString stringWithUTF8String:title] : @"";
        NSString* key = shortcut ? [NSString stringWithUTF8String:shortcut] : @"";
        NSMenuItem* item = [[NSMenuItem alloc] initWithTitle:t action:nil keyEquivalent:key];
        item.tag = command_id;
        [menu addItem:item];
    }
}

TTMacMenu tt_mac_menu_add_submenu(TTMacMenu parent, const char* title) {
    if (!parent) return NULL;
    @autoreleasepool {
        NSMenu* pmenu = (__bridge NSMenu*)parent;
        NSString* t = title ? [NSString stringWithUTF8String:title] : @"";
        NSMenu* submenu = [[NSMenu alloc] initWithTitle:t];
        NSMenuItem* item = [[NSMenuItem alloc] initWithTitle:t action:nil keyEquivalent:@""];
        item.submenu = submenu;
        [pmenu addItem:item];
        return (__bridge_retained void*)submenu;
    }
}

void tt_mac_menu_add_separator(TTMacMenu m) {
    if (!m) return;
    @autoreleasepool {
        NSMenu* menu = (__bridge NSMenu*)m;
        [menu addItem:[NSMenuItem separatorItem]];
    }
}

void tt_mac_menubar_set(TTMacMenu m) {
    if (!m) return;
    @autoreleasepool {
        NSMenu* menu = (__bridge NSMenu*)m;
        [NSApp setMainMenu:menu];
    }
}

/* --- Timer --- */

typedef struct {
    void (*callback)(void* ctx);
    void* ctx;
} TTTimerInfo;

@interface TTTimerTarget : NSObject
@property (nonatomic, assign) TTTimerInfo info;
- (void)timerFired:(NSTimer*)timer;
@end

@implementation TTTimerTarget
- (void)timerFired:(NSTimer*)timer {
    if (self.info.callback) {
        self.info.callback(self.info.ctx);
    }
}
@end

TTMacTimer tt_mac_timer_create(double interval_seconds, void (*callback)(void* ctx), void* ctx) {
    @autoreleasepool {
        TTTimerTarget* target = [[TTTimerTarget alloc] init];
        TTTimerInfo info = { callback, ctx };
        target.info = info;

        NSTimer* timer = [NSTimer scheduledTimerWithTimeInterval:interval_seconds
                                                          target:target
                                                        selector:@selector(timerFired:)
                                                        userInfo:nil
                                                         repeats:YES];
        return (__bridge_retained void*)timer;
    }
}

void tt_mac_timer_destroy(TTMacTimer t) {
    if (!t) return;
    @autoreleasepool {
        NSTimer* timer = (__bridge_transfer NSTimer*)t;
        [timer invalidate];
    }
}

/* --- Clipboard --- */

int tt_mac_clipboard_set_text(const char* text) {
    if (!text) return 0;
    @autoreleasepool {
        NSPasteboard* pb = [NSPasteboard generalPasteboard];
        [pb clearContents];
        NSString* str = [NSString stringWithUTF8String:text];
        return [pb setString:str forType:NSPasteboardTypeString] ? 1 : 0;
    }
}

int tt_mac_clipboard_set_text_w(const wchar_t* text) {
    if (!text) return 0;
    @autoreleasepool {
        NSPasteboard* pb = [NSPasteboard generalPasteboard];
        [pb clearContents];
        NSString* str = nsstring_from_wchar(text);
        if (!str) return 0;
        return [pb setString:str forType:NSPasteboardTypeString] ? 1 : 0;
    }
}

char* tt_mac_clipboard_get_text(void) {
    @autoreleasepool {
        NSPasteboard* pb = [NSPasteboard generalPasteboard];
        NSString* str = [pb stringForType:NSPasteboardTypeString];
        if (!str) return NULL;
        const char* utf8 = [str UTF8String];
        return strdup(utf8);
    }
}

wchar_t* tt_mac_clipboard_get_text_w(void) {
    @autoreleasepool {
        NSPasteboard* pb = [NSPasteboard generalPasteboard];
        NSString* str = [pb stringForType:NSPasteboardTypeString];
        if (!str) return NULL;

        NSData* data = [str dataUsingEncoding:NSUTF32LittleEndianStringEncoding];
        size_t wlen = [data length] / sizeof(wchar_t);
        wchar_t* result = (wchar_t*)malloc((wlen + 1) * sizeof(wchar_t));
        if (!result) return NULL;
        memcpy(result, [data bytes], wlen * sizeof(wchar_t));
        result[wlen] = L'\0';
        return result;
    }
}

void tt_mac_clipboard_free(void* ptr) {
    free(ptr);
}

/* --- Application --- */

@interface TTAppDelegate : NSObject <NSApplicationDelegate>
@end

@implementation TTAppDelegate

- (void)applicationDidFinishLaunching:(NSNotification *)notification {
    /* Application ready */
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)sender {
    return YES;
}

@end

void tt_mac_app_init(void) {
    @autoreleasepool {
        [NSApplication sharedApplication];
        TTAppDelegate* delegate = [[TTAppDelegate alloc] init];
        [NSApp setDelegate:delegate];
        [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
    }
}

void tt_mac_app_run(void) {
    @autoreleasepool {
        [NSApp activateIgnoringOtherApps:YES];
        [NSApp run];
    }
}

void tt_mac_app_quit(void) {
    @autoreleasepool {
        [NSApp terminate:nil];
    }
}

/* --- Message Box --- */

int tt_mac_messagebox(TTMacWindow parent, const char* text, const char* caption, unsigned int type) {
    @autoreleasepool {
        NSAlert* alert = [[NSAlert alloc] init];
        if (text)    alert.messageText = [NSString stringWithUTF8String:text];
        if (caption) alert.informativeText = [NSString stringWithUTF8String:caption];

        /* Map Windows MB_ types to NSAlert style */
        unsigned int icon = type & 0xF0;
        if (icon == 0x10) {       /* MB_ICONERROR */
            alert.alertStyle = NSAlertStyleCritical;
        } else if (icon == 0x30) { /* MB_ICONWARNING */
            alert.alertStyle = NSAlertStyleWarning;
        } else {
            alert.alertStyle = NSAlertStyleInformational;
        }

        unsigned int buttons = type & 0x0F;
        if (buttons == 1) {       /* MB_OKCANCEL */
            [alert addButtonWithTitle:@"OK"];
            [alert addButtonWithTitle:@"Cancel"];
        } else if (buttons == 4) { /* MB_YESNO */
            [alert addButtonWithTitle:@"Yes"];
            [alert addButtonWithTitle:@"No"];
        } else if (buttons == 3) { /* MB_YESNOCANCEL */
            [alert addButtonWithTitle:@"Yes"];
            [alert addButtonWithTitle:@"No"];
            [alert addButtonWithTitle:@"Cancel"];
        } else {
            [alert addButtonWithTitle:@"OK"];
        }

        NSModalResponse response = [alert runModal];
        /* Map back to Windows IDOK/IDCANCEL/IDYES/IDNO */
        if (response == NSAlertFirstButtonReturn)  return 1; /* IDOK / IDYES */
        if (response == NSAlertSecondButtonReturn) return 2; /* IDCANCEL / IDNO */
        if (response == NSAlertThirdButtonReturn)  return 7; /* IDNO (for YESNOCANCEL) */
        return 0;
    }
}

/* --- File Dialogs --- */

char* tt_mac_open_file_dialog(TTMacWindow parent, const char* filter, const char* initial_dir) {
    @autoreleasepool {
        NSOpenPanel* panel = [NSOpenPanel openPanel];
        panel.canChooseFiles = YES;
        panel.canChooseDirectories = NO;
        panel.allowsMultipleSelection = NO;

        if (initial_dir) {
            panel.directoryURL = [NSURL fileURLWithPath:[NSString stringWithUTF8String:initial_dir]];
        }

        if ([panel runModal] == NSModalResponseOK) {
            NSURL* url = panel.URL;
            return strdup([[url path] UTF8String]);
        }
        return NULL;
    }
}

char* tt_mac_save_file_dialog(TTMacWindow parent, const char* filter, const char* initial_dir) {
    @autoreleasepool {
        NSSavePanel* panel = [NSSavePanel savePanel];

        if (initial_dir) {
            panel.directoryURL = [NSURL fileURLWithPath:[NSString stringWithUTF8String:initial_dir]];
        }

        if ([panel runModal] == NSModalResponseOK) {
            NSURL* url = panel.URL;
            return strdup([[url path] UTF8String]);
        }
        return NULL;
    }
}
