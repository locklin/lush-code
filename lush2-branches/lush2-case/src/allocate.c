/***********************************************************************
 * 
 *  LUSH Lisp Universal Shell
 *    Copyright (C) 2009 Leon Bottou, Yann Le Cun, Ralf Juengling.
 *    Copyright (C) 2002 Leon Bottou, Yann Le Cun, AT&T Corp, NECI.
 *  Includes parts of TL3:
 *    Copyright (C) 1987-1999 Leon Bottou and Neuristique.
 *  Includes selected parts of SN3.2:
 *    Copyright (C) 1991-2001 AT&T Corp.
 * 
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the Lesser GNU General Public License as 
 *  published by the Free Software Foundation; either version 2 of the
 *  License, or (at your option) any later version.
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
  This is a generic package for allocation and deallocation
  with a free list. The objects and the allocation structure
  are described in a  'generic_alloc structure'
  Contains also the garbage collector...
********************************************************************** */

#include "header.h"
#include "allocate.h"

#define SIZEOF_CHUNK_AT (SIZEOF_CELL*CONSCHUNKSIZE)

void allocate_chunk(alloc_root_t *ar)
{
  
  assert(ar->freelist==NULL);
  size_t sizeof_cell = ar->sizeof_elem + 2*REDZONE_SIZE;
  size_t sizeof_chunk = sizeof_cell*ar->chunksize;
  chunk_header_t *chkhd = malloc(sizeof(chunk_header_t) + sizeof_chunk);
  //printf("adding new chunk of size %d\n", ar->chunksize);
  if (chkhd) {
    chkhd->begin = (empty_alloc_t *) ((char*)chkhd + sizeof(chunk_header_t));
    chkhd->end = (empty_alloc_t *) ((char*)chkhd->begin + sizeof_chunk);
    chkhd->next = ar->chunklist;
    ar->chunklist = chkhd;
    
    char *p = (char*)chkhd->begin + REDZONE_SIZE;
    for (int i = 0; i < ar->chunksize; i++, p+=sizeof_cell) {
      empty_alloc_t *ea = (empty_alloc_t *)p;
      ea->next = ar->freelist;
      ea->used = 0;
      ar->freelist = ea;
    }
    VALGRIND_MAKE_MEM_NOACCESS(chkhd->begin, sizeof_chunk);
  } else
    RAISEF("not enough memory", NIL);
}
