/***********************************************************************
 * 
 *  LUSH Lisp Universal Shell
 *    Copyright (C) 2002 Leon Bottou, Yann Le Cun, AT&T Corp, NECI.
 *  Includes parts of TL3:
 *    Copyright (C) 1987-1999 Leon Bottou and Neuristique.
 *  Includes selected parts of SN3.2:
 *    Copyright (C) 1991-2001 AT&T Corp.
 * 
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 * 
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 * 
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA
 * 
 ***********************************************************************/

/***********************************************************************
 * $Id: dldbfd.h,v 1.2 2002-05-01 15:23:00 leonb Exp $
 **********************************************************************/

/* DLD-4.0 -- DYNAMIC LINK/UNLINK EDITOR FOR C
 * -------------------------------------------
 *
 * DESCRIPTION
 * 
 *   DLD-4.0 implements the same functionalities than DLD-3.2.3.
 *   Unlike DLD-3.2.3 however, DLD-4.0 does not rely on machine
 *   specific code but makes extensive use of the GNU "BFD" library.
 *   This allows us to easily port DLD on new platforms.
 *
 * LIMITATIONS OF DLD-4.0
 *
 * - Ignore warnings attached to symbols.
 * - Ignore constructor/destructor sections.
 * - Support for indirect symbols is untested.
 * - Weakly defined symbols are partially supported (one way!)
 * - Common symbols are defined in the bss section of the first referencing file
 * - No relaxation is performed ever.
 *
 */


/* HERE IS THE COPYRIGHT NOTICE INCLUDED WITH DLD-3.2.3 
 * ----------------------------------------------------
 *
 * DLD-3.2.3 Copyright (C) 1990 by W. Wilson Ho.
 *
 * The author can be reached electronically by how@cs.ucdavis.edu or
 * through physical mail at:
 *
 * W. Wilson Ho
 * Division of Computer Science
 * University of California at Davis
 * Davis, CA 95616
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 1, or (at your option) any later
 * version.
 *
 * This program borrows and modifies a number of functions and data structures
 * from the implementation of the GNU `ld' link editor.  The original copyleft
 * notice from the GNU `ld' is also included.
 *
 * Linker `ld' for GNU
 * Copyright (C) 1988 Free Software Foundation, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 1, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  
 */


extern const char *dld_errno;
extern int dld_undefined_sym_count;
extern int dld_compatibility_flag;

int dld_init (const char *);
int dld_link (const char *);
void *dld_get_symbol (const char *);
void *dld_get_func (const char *);
void *dld_get_bare_symbol (const char *);
int dld_unlink_by_file (const char *, int);
int dld_unlink_by_symbol (const char *, int);
int dld_function_executable_p (const char *);
const char **dld_list_undefined_sym (void);
char *dld_find_executable (const char *);
int dld_create_reference (const char *);
int dld_define_sym (const char *, size_t);
void dld_remove_defined_symbol (const char *);
void dld_perror (const char *);
void *dld_dlopen(char *path, int mode); /* NEW */ 




