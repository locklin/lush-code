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

/* Portions of CPS.h */
typedef struct CPSProcessSerNum { UInt32 lo; UInt32 hi; } CPSProcessSerNum;
extern OSErr CPSGetCurrentProcess( CPSProcessSerNum *psn);
extern OSErr CPSSetFrontProcess( CPSProcessSerNum *psn);
extern OSErr CPSEnableForegroundOperation( CPSProcessSerNum *psn, 
					   UInt32, UInt32, UInt32, UInt32);

/* Misc */
static int gInitialized;
static sigjmp_buf gBackToWork;

@interface SDLApplication : NSApplication
@end

@implementation SDLApplication
- (void)terminate:(id)sender
{
    /* Post a SDL_QUIT event */
    SDL_Event event;
    event.type = SDL_QUIT;
    SDL_PushEvent(&event);
}
@end

@interface SDLMain : NSObject
@end

@implementation SDLMain
- (void) applicationDidFinishLaunching: (NSNotification *) note
{
    siglongjmp(gBackToWork,1);
}
@end

@interface NSAppleMenuController:NSObject {}
- (void)controlMenu:(NSMenu *)aMenu;
@end

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

void setupWindowMenu(void)
{
    NSMenu	*windowMenu;
    NSMenuItem	*windowMenuItem;
    NSMenuItem	*menuItem;
    windowMenu = [[NSMenu alloc] initWithTitle:@"Window"];
    menuItem = [[NSMenuItem alloc] initWithTitle:@"Minimize" 
				   action:@selector(performMiniaturize:) 
				   keyEquivalent:@"m"];
    [windowMenu addItem:menuItem];
    [menuItem release];
    windowMenuItem = [[NSMenuItem alloc] initWithTitle:@"Window" 
					 action:nil keyEquivalent:@""];
    [windowMenuItem setSubmenu:windowMenu];
    [[NSApp mainMenu] addItem:windowMenuItem];
    [NSApp setWindowsMenu:windowMenu];
    [windowMenu release];
    [windowMenuItem release];
}

void SDLmain (void)
{
  CPSProcessSerNum PSN;
  NSAutoreleasePool *pool;
  SDLMain *sdlMain;
  if (!gInitialized)
    {
      gInitialized = 1;
      if (sigsetjmp(gBackToWork,1))
	return;
      pool = [[NSAutoreleasePool alloc] init];
      [SDLApplication sharedApplication];
      /* Tell the dock about us */
      if (!CPSGetCurrentProcess(&PSN))
	if (!CPSEnableForegroundOperation(&PSN,0x03,0x3C,0x2C,0x1103))
	  if (!CPSSetFrontProcess(&PSN))
	    [SDLApplication sharedApplication];
#if THIS_DOES_NOTHING_I_CAN_SEE
      [NSApp setMainMenu:[[NSMenu alloc] init]];
      setupAppleMenu();
      setupWindowMenu();
#endif
      sdlMain = [[SDLMain alloc] init];
      [NSApp setDelegate:sdlMain];
#if THIS_DOES_NOT_WORK
      [NSApp run];
      [sdlMain release];
      [pool release];
      gInitialized = 0;
#endif
    }
}
