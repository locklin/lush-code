/*   TL3 Lisp interpreter and development tools
 *   Copyright (C) 1991-1999 Leon Bottou and Neuristique.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
 
/************************************************************************
   WinTL3:   (C) LYB 1996
   win32.h -- define communication between DLL and main.
   $Id: win32.h,v 1.1.1.1 2002-04-16 17:37:33 leonb Exp $
*********************************************************************** */




/* -------------------------------------------
   C++ STUFF
   ------------------------------------------- */

#ifdef __cplusplus
extern "C" {
#endif


/* -------------------------------------------
   COMPATIBILITY WITH VC++ 4.0 and 4.1
   ------------------------------------------- */

#if (defined(_MSC_VER) && _MSC_VER<1020)
#pragma comment(linker, "/nodefaultlib:msvcrtd.lib")
#pragma comment(linker, "/nodefaultlib:msvcrt.lib")
#pragma comment(lib, "./open/msvc/msvcrt42.lib")
#endif


/* -------------------------------------------
   MESSAGE PASSING STRUCTURE
   ------------------------------------------- */

/* 
 *  TL3MSG -- enumerated type defining each message
 */

typedef enum TL3MSG_tag 
{
  TL3_EXIT,    
  TL3_WINEDIT,
  TL3_GETLINE,
  TL3_WAITEVENT,
  TL3_CREATEWIN,
  TL3_CREATETOPWIN,
  TL3_CLOSEWIN,
  TL3_UPDATEWIN,
  TL3_CONFIGWIN,
  TL3_QUERYWIN,
  TL3_HILITEWIN,
  TL3_SHOWWORKBENCH,
}
TL3MSG;


/* 
 * TL3ARG -- union for message arguments and replies
 */

struct msg_simple 
{
    int status;
};

struct msg_string 
{
    int status;
    const char *string;
};

struct msg_createwin 
{
    int status;
    int x, y, w, h;
    const char *title;
    HDC hdc;
    CRITICAL_SECTION *lock;
    void *wid;
    HPALETTE hpal;
    LOGPALETTE *logpal;
};

struct msg_configwin 
{
    int status;
    void *wid;
    int x, y, w, h;
    void *hwnd;
};

struct msg_simplewin 
{
    int status;
    void *wid;
};


typedef union TL3ARG_tag 
{
  struct msg_simple arg_simple;
  struct msg_string arg_string;
  struct msg_createwin arg_createwin;
  struct msg_configwin arg_configwin;
  struct msg_simplewin arg_simplewin;
}
TL3ARG;

/* generic status code */
#define MSG_ERROR          -1
#define MSG_OK              0
/* status code for EDIT */
#define MSG_EDIT_AUTO       0
#define MSG_EDIT_UNNAMED    1
#define MSG_EDIT_NAMED      2
/* status code for GETLINE */
#define MSG_GETLINE_NORMAL  0    
#define MSG_GETLINE_EVENT   1
#define MSG_GETLINE_EOF     2
/* status code for CONFIGWIN and QUERYWIN */
#define MSG_CONFIGWIN_X     1
#define MSG_CONFIGWIN_Y     2
#define MSG_CONFIGWIN_W     4
#define MSG_CONFIGWIN_H     8
#define MSG_CONFIGWIN_RAISE 16
#define MSG_CONFIGWIN_ICON  32


/*
 * TL3PORT -- structure for synchronizing communications
 */

typedef struct TL3PORT_tag 
{
    HANDLE hfStdoutInheritable;  // file handle linked to WinTL3 console
    HANDLE hevSendMessage;       // set by TL3DLL to initiate RPC
    HANDLE hevReplyMessage;      // set by WinTL3 to reply RPC
    TL3MSG msgcode;		 // -- RPC id
    TL3ARG *msgarg;		 // -- RPC arguments
} 
TL3PORT;



/* -------------------------------------------
   ELEMENTARY DEFINITIONS
   ------------------------------------------- */

#if !defined(TLAPI)
#if defined(_CONSOLE)
#define TLAPI /**/
#elif defined(TL3DLL)
#define TLAPI __declspec(dllexport)
#else  /* !defined TL3DLL && !defined(_CONSOLE) */
#define TLAPI __declspec(dllimport)
#endif /* !defined(TL3DLL) */
#endif /* !defined(TLAPI) */


extern TLAPI int break_attempt;
extern TLAPI int kill_attempt;
extern TLAPI TL3PORT *tl3port;

TLAPI int  tl3main(int, char**);
TLAPI int  tl3exception(LPEXCEPTION_POINTERS);
TLAPI void tl3send(TL3MSG msgcode, TL3ARG *msgarg);
TLAPI BOOL tl3event_test(void);
TLAPI BOOL tl3event_add(void *wid, int code, int arg, int x, int y);



/* -------------------------------------------
   C++ STUFF
   ------------------------------------------- */

#ifdef __cplusplus
}
#endif


