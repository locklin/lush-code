/* Source code for the TL3 command */

#include <windows.h>
#include "win32.h"


/* Just a stub for TL3DLL.DLL */

int main(int argc, char **argv)
{
  __try 
  {
    return tl3main(argc, argv);
  } 
  __except (tl3exception(GetExceptionInformation()))
  {
    return GetExceptionCode();
  }
}


