
#include "header.h"

bool integerp(double d)
{
   return d == nearbyint(d);
}

DX(xintegerp)
{
   ARG_NUMBER(1);
   return NEW_BOOL(integerp(AREAL(1)));
}


bool oddp(double d)
{
   ifn (integerp(d))
      RAISEF("not an integer", NEW_NUMBER(d));
   return d/2.0 != nearbyint(d/2.0);
}

DX(xoddp)
{
   ARG_NUMBER(1);
   return NEW_BOOL(oddp(AREAL(1)));
}


bool evenp(double d)
{
   ifn (integerp(d))
      RAISEF("not an integer", NEW_NUMBER(d));
   return d/2.0 == nearbyint(d/2.0);
}

DX(xevenp)
{
   ARG_NUMBER(1);
   return NEW_BOOL(evenp(AREAL(1)));
}


void init_number(void)
{
   dx_define("integerp", xintegerp);
   dx_define("oddp", xoddp);
   dx_define("evenp", xevenp);
}


/* -------------------------------------------------------------
   Local Variables:
   c-file-style: "k&r"
   c-basic-offset: 3
   End:
   ------------------------------------------------------------- */
