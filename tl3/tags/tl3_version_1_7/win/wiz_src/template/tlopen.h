
#ifndef TLOPEN_H
#define TLOPEN_H

/* ---------------------------------
 * This file simply loads the TL3 
 * header file under Unix. The proper
 * directories are specified by the
 * compiler options.
 */
#ifndef WIN32
#include "header.h"
#else /* WIN32 */

/* ---------------------------------
 * Specify TL3 header file (windows)
 */
$$IF(!WINTL3DIR)
// <<<<<WARNING>>>>
// You must edit the following line and replace
// "C:\Progra~1\WinTL3\TL3" by the TL3 directory name.
$$ENDIF
#include "$$TLOPENINCLUDE$$/header.h"


/* ---------------------------------
 * Specify TL/Open library file (windows)
 */
$$IF(!WINTL3DIR)
// <<<<<WARNING>>>>
// You must edit the following line and replace
// "C:\Progra~1\WinTL3\TL3" by the TL3 directory name.
// You must use a short path name here (without spaces).
$$ENDIF
#pragma comment(lib,"$$TLOPENLIB$$/tl3dll.lib")


/* ---------------------------------
 * Check that TL/Open extension is
 * compiled with the correct options.
 **** You may need to edit thje wintl3 directory name.
 */

#if (!defined(_MSC_VER) || _MSC_VER<1000)
#error "Microsoft Visual C++ 4.x compiler is required"
#elif (_MSC_VER<1020)
#pragma comment(linker, "/nodefaultlib:msvcrtd.lib")
#pragma comment(linker, "/nodefaultlib:msvcrt.lib")
$$IF(!WINTL3DIR)
// <<<<<WARNING>>>>
// You must edit the following line and replace
// "C:\Program Files\WinTL3\TL3" by the TL3 directory name.
$$ENDIF
#pragma comment(lib, "$$TLOPENLIB$$/msvcrt42.lib")
#elif (defined(_DEBUG))
#pragma comment(linker, "/nodefaultlib:msvcrtd.lib")
#pragma comment(lib, "msvcrt.lib")
#endif
#if (!defined(_MT) && !defined(_DLL))
#error "TLOpen project requires compiler option /MD"
#endif


/* --------------------------------- */
#endif /* WIN32    */
#endif /* TLOPEN_H */
