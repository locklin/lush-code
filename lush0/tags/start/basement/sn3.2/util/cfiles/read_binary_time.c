
#include "header.h"

#include <sys/types.h>
#include <sys/time.h>
#define MAX_DESCR 4194303             /* as returned by ulimit()  */

/**************************************************************************/
/*                                                                        */
/*  Reads a lisp object from the binary file                              */
/*                                                                        */
/*  Parameter: filename or file descriptor to be read, timeout value      */
/*  Returns: at * - answer read from file                                 */
/*                                                                        */
/**************************************************************************/

DX(xread_binary_time) {
    at *p = NIL;                              /* LISP cons cell descriptor */
    File *f;                                                  /* object fd */
    at *ans;                              /* CONS structure to be returned */
    struct rtable *rt;
    int time_value;
    
    fd_set read_set;                        /* file descriptors to be read */
    struct timeval time_out;              /* timeout value on select calls */
    int result;                                          /* of select call */


    if (arg_number==0) {
	p = NIL;
	rt = NIL;
	ifn (f = context->input_file)                  /* context?  stdin? */
	    error(NIL,"no input file!",NIL);
    } 
    else {
	ARG_NUMBER(2);                 
	ARG_EVAL(1);
	ARG_EVAL(2);
	p = APOINTER(1);                                 /* the file passed */
	time_value = AINTEGER(2);

	if (ISSTRING(1)) {        
	    p = OPEN_READ(ASTRING(1), NIL);       /* open read ==> new_extern 
						     returns a LISP descriptor
						     for an EXTERNAL object */
	    f = p->Object;
	    rt = NIL;
	}

	else                              
	    if (p && (p->ctype==XT_FILE_RO)) {
		LOCK(p);                                      /* incr count */
		f = p->Object;                    
		rt = NIL;
	    } 
	    else                                 
		if (p && (p->ctype==XT_RTABLE)) {
		    LOCK(p);
		    p = APOINTER(1);                /* file to be read from */
		    rt = p->Object;
		    ifn (f = context->input_file)
			error(NIL,"not a file",p);
		}
    }

    time_out.tv_usec = 0;                                   /* microseconds */
    time_out.tv_sec = time_value;                                /* seconds */

    
    FD_ZERO(&read_set);                    /* initialize the fd set to null */
    FD_SET(fileno(f->stream.fp),&read_set);                /* add current fd to fd set */


                        /* select returns -1 on failure, */
                        /*                 0 if timeout expired, */
			/*		num of descriptors ready on success */

    if (f->stream.fp->_cnt == 0 &&
	((result = select(MAX_DESCR,&read_set, NULL, NULL, &time_out)) <= 0)){

	if (result == -1) perror("read_binary_time: select failed");
	return NULL;
    }
    else {  
	ans = in_at_rt(f,rt);
	UNLOCK(p);
	return ans;
    }
}




/***************************************/

void init_read_binary_time()
{
  dx_define("read-binary-time",xread_binary_time);

}

int majver_read_binary_time = 3;
int minver_read_binary_time = 1;

