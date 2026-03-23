/*
 * Copyright (C) 2024 TeraTerm Project
 * All rights reserved.
 *
 * macOS main entry point for Tera Term.
 * Replaces the Windows WinMain with a Cocoa NSApplication loop.
 */

#import <Cocoa/Cocoa.h>
#include "macos_window.h"
#include "macos_pty.h"
#include "macos_net.h"

/* Forward declarations for terminal core */
/* These will be linked from the shared terminal emulation code */

/* --- Terminal View Controller --- */
@interface TTTerminalViewController : NSObject
@property (nonatomic, assign) TTMacWindow window;
@property (nonatomic, assign) TTMacView termView;
@property (nonatomic, assign) TTMacPty pty;
@property (nonatomic, strong) NSTimer *readTimer;
@end

@implementation TTTerminalViewController

- (instancetype)init {
    self = [super init];
    if (self) {
        _window = NULL;
        _termView = NULL;
        _pty = NULL;
    }
    return self;
}

- (void)createWindowWithTitle:(NSString *)title cols:(int)cols rows:(int)rows {
    /* Calculate window size based on font metrics */
    TTMacFont font = tt_mac_font_create("Menlo", 14, 0, 0);
    int charWidth = 0, charHeight = 0, ascent = 0;
    tt_mac_font_get_metrics(font, &charWidth, &charHeight, &ascent);
    tt_mac_font_destroy(font);

    int width = cols * charWidth + 20;   /* margins */
    int height = rows * charHeight + 20;

    self.window = tt_mac_window_create(100, 100, width, height,
                                        [title UTF8String]);
    self.termView = tt_mac_termview_create(self.window);
    tt_mac_termview_set_font(self.termView, "Menlo", 14, 0, 0);
    tt_mac_termview_set_colors(self.termView,
                                0x00FFFFFF,  /* white foreground */
                                0x00000000); /* black background */
}

- (void)startLocalShell {
    self.pty = tt_mac_pty_create(NULL, 80, 24);
    if (self.pty) {
        /* Start reading from PTY */
        self.readTimer = [NSTimer scheduledTimerWithTimeInterval:0.01
                                                          target:self
                                                        selector:@selector(readFromPty:)
                                                        userInfo:nil
                                                         repeats:YES];
    }
}

- (void)readFromPty:(NSTimer *)timer {
    if (!self.pty) return;

    char buffer[4096];
    int n = tt_mac_pty_read(self.pty, buffer, sizeof(buffer) - 1);
    if (n > 0) {
        buffer[n] = '\0';
        /* TODO: Feed data to VT terminal emulation engine (vtterm.c) */
        /* For now, this is a placeholder - the terminal emulation core
         * will process escape sequences and update the display buffer */
        tt_mac_termview_invalidate(self.termView);
    } else if (n < 0) {
        /* PTY closed or error */
        [self.readTimer invalidate];
        self.readTimer = nil;
    }

    if (!tt_mac_pty_is_alive(self.pty)) {
        [self.readTimer invalidate];
        self.readTimer = nil;
    }
}

- (void)connectTCP:(const char *)host port:(int)port {
    int sock = tt_mac_tcp_connect(host, port);
    if (sock >= 0) {
        /* TODO: Set up telnet/SSH session over this socket */
        /* This will integrate with commlib.c / telnet.c / ttssh */
    }
}

- (void)dealloc {
    if (self.readTimer) {
        [self.readTimer invalidate];
    }
    if (self.pty) {
        tt_mac_pty_destroy(self.pty);
    }
    if (self.window) {
        tt_mac_window_destroy(self.window);
    }
}

@end

/* --- Application Delegate --- */
@interface TTMacAppDelegate : NSObject <NSApplicationDelegate>
@property (nonatomic, strong) TTTerminalViewController *termController;
@end

@implementation TTMacAppDelegate

- (void)applicationDidFinishLaunching:(NSNotification *)notification {
    [self setupMenuBar];

    self.termController = [[TTTerminalViewController alloc] init];
    [self.termController createWindowWithTitle:@"Tera Term" cols:80 rows:24];

    tt_mac_window_show(self.termController.window);

    /* Start a local shell session by default */
    [self.termController startLocalShell];
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)sender {
    return YES;
}

- (void)setupMenuBar {
    NSMenu *mainMenu = [[NSMenu alloc] init];

    /* Application menu */
    NSMenuItem *appMenuItem = [[NSMenuItem alloc] init];
    NSMenu *appMenu = [[NSMenu alloc] init];
    [appMenu addItemWithTitle:@"About Tera Term"
                       action:@selector(showAbout:)
                keyEquivalent:@""];
    [appMenu addItem:[NSMenuItem separatorItem]];
    [appMenu addItemWithTitle:@"Preferences..."
                       action:@selector(showPreferences:)
                keyEquivalent:@","];
    [appMenu addItem:[NSMenuItem separatorItem]];
    [appMenu addItemWithTitle:@"Quit Tera Term"
                       action:@selector(terminate:)
                keyEquivalent:@"q"];
    appMenuItem.submenu = appMenu;
    [mainMenu addItem:appMenuItem];

    /* File menu */
    NSMenuItem *fileMenuItem = [[NSMenuItem alloc] init];
    NSMenu *fileMenu = [[NSMenu alloc] initWithTitle:@"File"];
    [fileMenu addItemWithTitle:@"New Connection..."
                        action:@selector(newConnection:)
                 keyEquivalent:@"n"];
    [fileMenu addItemWithTitle:@"New Local Shell"
                        action:@selector(newLocalShell:)
                 keyEquivalent:@"t"];
    [fileMenu addItem:[NSMenuItem separatorItem]];
    [fileMenu addItemWithTitle:@"Log..."
                        action:@selector(startLog:)
                 keyEquivalent:@"l"];
    [fileMenu addItem:[NSMenuItem separatorItem]];
    [fileMenu addItemWithTitle:@"Close"
                        action:@selector(performClose:)
                 keyEquivalent:@"w"];
    fileMenuItem.submenu = fileMenu;
    [mainMenu addItem:fileMenuItem];

    /* Edit menu */
    NSMenuItem *editMenuItem = [[NSMenuItem alloc] init];
    NSMenu *editMenu = [[NSMenu alloc] initWithTitle:@"Edit"];
    [editMenu addItemWithTitle:@"Copy"
                        action:@selector(copy:)
                 keyEquivalent:@"c"];
    [editMenu addItemWithTitle:@"Paste"
                        action:@selector(paste:)
                 keyEquivalent:@"v"];
    [editMenu addItemWithTitle:@"Select All"
                        action:@selector(selectAll:)
                 keyEquivalent:@"a"];
    [editMenu addItem:[NSMenuItem separatorItem]];
    [editMenu addItemWithTitle:@"Find..."
                        action:@selector(performFindPanelAction:)
                 keyEquivalent:@"f"];
    editMenuItem.submenu = editMenu;
    [mainMenu addItem:editMenuItem];

    /* Setup menu */
    NSMenuItem *setupMenuItem = [[NSMenuItem alloc] init];
    NSMenu *setupMenu = [[NSMenu alloc] initWithTitle:@"Setup"];
    [setupMenu addItemWithTitle:@"Terminal..."
                         action:@selector(setupTerminal:)
                  keyEquivalent:@""];
    [setupMenu addItemWithTitle:@"Window..."
                         action:@selector(setupWindow:)
                  keyEquivalent:@""];
    [setupMenu addItemWithTitle:@"Font..."
                         action:@selector(setupFont:)
                  keyEquivalent:@""];
    [setupMenu addItemWithTitle:@"Keyboard..."
                         action:@selector(setupKeyboard:)
                  keyEquivalent:@""];
    [setupMenu addItemWithTitle:@"Serial Port..."
                         action:@selector(setupSerial:)
                  keyEquivalent:@""];
    setupMenuItem.submenu = setupMenu;
    [mainMenu addItem:setupMenuItem];

    /* Help menu */
    NSMenuItem *helpMenuItem = [[NSMenuItem alloc] init];
    NSMenu *helpMenu = [[NSMenu alloc] initWithTitle:@"Help"];
    [helpMenu addItemWithTitle:@"Tera Term Help"
                        action:@selector(showHelp:)
                 keyEquivalent:@"?"];
    helpMenuItem.submenu = helpMenu;
    [mainMenu addItem:helpMenuItem];

    [NSApp setMainMenu:mainMenu];
}

- (void)showAbout:(id)sender {
    tt_mac_messagebox(NULL,
                      "Tera Term for macOS (ARM64)\nBased on Tera Term Project\nhttps://teratermproject.github.io/",
                      "About Tera Term", 0);
}

- (void)showPreferences:(id)sender {
    /* TODO: Open preferences window */
}

- (void)newConnection:(id)sender {
    /* TODO: Show connection dialog (host/port/protocol) */
}

- (void)newLocalShell:(id)sender {
    /* TODO: Open new terminal with local shell */
}

- (void)startLog:(id)sender {
    /* TODO: Start logging */
}

- (void)setupTerminal:(id)sender { /* TODO */ }
- (void)setupWindow:(id)sender { /* TODO */ }
- (void)setupFont:(id)sender { /* TODO */ }
- (void)setupKeyboard:(id)sender { /* TODO */ }
- (void)setupSerial:(id)sender { /* TODO */ }
- (void)showHelp:(id)sender { /* TODO */ }

@end

/* --- Main entry point --- */
int main(int argc, const char *argv[]) {
    @autoreleasepool {
        [NSApplication sharedApplication];

        TTMacAppDelegate *delegate = [[TTMacAppDelegate alloc] init];
        [NSApp setDelegate:delegate];
        [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];

        /* Initialize networking */
        tt_mac_net_init();

        [NSApp activateIgnoringOtherApps:YES];
        [NSApp run];

        tt_mac_net_cleanup();
    }
    return 0;
}
