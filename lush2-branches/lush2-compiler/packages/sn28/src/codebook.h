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
 *  This file is derived from SN-2.8
 *    Copyright (C) 1987-1999 Neuristique s.a.
 ***********************************************************************/

/***********************************************************************
 * $Id: codebook.h,v 1.1 2003/03/18 00:52:22 leonb Exp $
 **********************************************************************/


/* ---------- CODEBOOK ---------- */

struct codeword {
  flt *word;
  int label;
};

struct codebook {
  int ncode;
  int ndim;
  struct codeword *code;
  at *word_matrix;
};


extern class codebook_class;

at *new_codebook(int ncode, int ndim);
at *split_codebook(at *p, int i, int j);
at *merge_codebook(int n, at **atcbs);
at *select_codebook(int n, at **atcbs, int (*test) (int));

struct codebook *check_codebook(at *p);


/* ---------- ADAPTKNN ---------- */


int *knn(float *x, struct codebook *cb, int k, int *res);

flt perf_knn(struct codebook *cbtst, struct codebook *cbref, int knn);

at *k_means(struct codebook *cb, int km);

void learn_lvq1(struct codebook *cbdata, struct codebook *cbref, 
		int nbit, float a0);

void learn_lvq2(struct codebook *cbdata, struct codebook *cbref, 
		int nbit, float a0, float win);

void learn_lvq3(struct codebook *cbdata, struct codebook *cbref, 
		int nbit, float a0, float win);

