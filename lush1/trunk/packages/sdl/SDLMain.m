/*   SDLMain.m - main entry point for our Cocoa-ized SDL app
       Initial Version: Darrell Walisser <dwaliss1@purdue.edu>
       Non-NIB-Code & other changes: Max Horn <max@quendi.de>

    Feel free to customize this file to suit your needs
    Customized for Lush...
*/

#import <SDL/SDL.h>
#import <sys/param.h> /* for MAXPATHLEN */
#import <unistd.h>
#import <setjmp.h>
#import <Cocoa/Cocoa.h>

@interface SDLMain : NSObject
@end

static int gInitialized;
static sigjmp_buf gBackToWork;

/* An internal Apple class used to setup Apple menus */
@interface NSAppleMenuController:NSObject {}
- (void)controlMenu:(NSMenu *)aMenu;
@end

@interface SDLApplication : NSApplication
@end

@implementation SDLApplication
/* Invoked from the Quit menu item */
- (void)terminate:(id)sender
{
    /* Post a SDL_QUIT event */
    SDL_Event event;
    event.type = SDL_QUIT;
    SDL_PushEvent(&event);
}
@end

@implementation SDLMain

void setupAppleMenu(void)
{
    /* warning: this code is very odd */
    NSAppleMenuController *appleMenuController;
    NSMenu *appleMenu;
    NSMenuItem *appleMenuItem;
    appleMenuController = [[NSAppleMenuController alloc] init];
    appleMenu = [[NSMenu alloc] initWithTitle:@""];
    appleMenuItem = [[NSMenuItem alloc] initWithTitle:@"" 
	action:nil keyEquivalent:@""];
    [appleMenuItem setSubmenu:appleMenu];
    [[NSApp mainMenu] addItem:appleMenuItem];
    [appleMenuController controlMenu:appleMenu];
    [[NSApp mainMenu] removeItem:appleMenuItem];
    [appleMenu release];
    [appleMenuItem release];
}

/* Create a window menu */
void setupWindowMenu(void)
{
    NSMenu		*windowMenu;
    NSMenuItem	*windowMenuItem;
    NSMenuItem	*menuItem;


    windowMenu = [[NSMenu alloc] initWithTitle:@"Window"];
    
    /* "Minimize" item */
    menuItem = [[NSMenuItem alloc] initWithTitle:@"Minimize" 
				   action:@selector(performMiniaturize:) 
				   keyEquivalent:@"m"];
    [windowMenu addItem:menuItem];
    [menuItem release];
    
    /* Put menu into the menubar */
    windowMenuItem = [[NSMenuItem alloc] initWithTitle:@"Window" 
					 action:nil 
					 keyEquivalent:@""];
    [windowMenuItem setSubmenu:windowMenu];
    [[NSApp mainMenu] addItem:windowMenuItem];
    
    /* Tell the application object that this is now the window menu */
    [NSApp setWindowsMenu:windowMenu];

    /* Finally give up our references to the objects */
    [windowMenu release];
    [windowMenuItem release];
}



/* Called when the internal event loop has just started running */
- (void) applicationDidFinishLaunching: (NSNotification *) note
{
    /* Hand off to main application code */
    siglongjmp(gBackToWork,1);
}
@end


/* Replacement for NSApplicationMain */
void SDLmain (void)
{
  NSAutoreleasePool *pool;
  SDLMain *sdlMain;
  if (!gInitialized)
    {
      gInitialized = 1;
      if (sigsetjmp(gBackToWork,1))
	return;
      pool = [[NSAutoreleasePool alloc] init];
      [SDLApplication sharedApplication];
      [NSApp setMainMenu:[[NSMenu alloc] init]];
      setupAppleMenu();
      setupWindowMenu();
      sdlMain = [[SDLMain alloc] init];
      [NSApp setDelegate:sdlMain];
      [NSApp run];
      [sdlMain release];
      [pool release];
      gInitialized = 0;
    }
}
