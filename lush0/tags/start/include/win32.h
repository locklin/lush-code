/*   Lush Lisp interpreter and development tools
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
   WinT-Lisp3:   (C) LYB 1996
   win32.h -- define communication between DLL and main.
   $Id: win32.h,v 0.1.1.1 2001-10-31 17:29:34 profshadoko Exp $
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
 *  LushMSG -- enumerated type defining each message
 */

typedef enum LushMSG_tag 
{
  LUSH_EXIT,    
  LUSH_WINEDIT,
  LUSH_GETLINE,
  LUSH_WAITEVENT,
  LUSH_CREATEWIN,
  LUSH_CREATETOPWIN,
  LUSH_CLOSEWIN,
  LUSH_UPDATEWIN,
  LUSH_CONFIGWIN,
  LUSH_QUERYWIN,
  LUSH_HILITEWIN,
  LUSH_SHOWWORKBENCH,
}
LushMSG;


/* 
 * LUSHARG -- union for message arguments and replies
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


typedef union LUSHARG_tag 
{
  struct msg_simple arg_simple;
  struct msg_string arg_string;
  struct msg_createwin arg_createwin;
  struct msg_configwin arg_configwin;
  struct msg_simplewin arg_simplewin;
}
LUSHARG;

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
 * LushPORT -- structure for synchronizing communications
 */

typedef struct LushPORT_tag 
{
    HANDLE hfStdoutInheritable;  // file handle linked to WinLush console
    HANDLE hevSendMessage;       // set by LushDLL to initiate RPC
    HANDLE hevReplyMessage;      // set by WinLush to reply RPC
    LushMSG msgcode;		 // -- RPC id
    LUSHARG *msgarg;		 // -- RPC arguments
} 
LushPORT;



/* -------------------------------------------
   ELEMENTARY DEFINITIONS
   ------------------------------------------- */

#if !defined(LUSHAPI)
#if defined(_CONSOLE)
#define LUSHAPI /**/
#elif defined(LushDLL)
#define LUSHAPI __declspec(dllexport)
#else  /* !defined LushDLL && !defined(_CONSOLE) */
#define LUSHAPI __declspec(dllimport)
#endif /* !defined(LushDLL) */
#endif /* !defined(LUSHAPI) */


extern LUSHAPI int break_attempt;
extern LUSHAPI int kill_attempt;
extern LUSHAPI LushPORT *lushport;

LUSHAPI int  lushmain(int, char**);
LUSHAPI int  lushexception(LPEXCEPTION_POINTERS);
LUSHAPI void lushsend(LushMSG msgcode, LUSHARG *msgarg);
LUSHAPI BOOL lushevent_test(void);
LUSHAPI BOOL lushevent_add(void *wid, int code, int arg, int x, int y);



/* -------------------------------------------
   C++ STUFF
   ------------------------------------------- */

#ifdef __cplusplus
}
#endif


