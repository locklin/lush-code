/***********************************************************************
 * 
 *  LUSH Lisp Universal Shell
 *    Copyright (C) 2009 Leon Bottou, Yann LeCun, Ralf Juengling.
 *    Copyright (C) 2002 Leon Bottou, Yann LeCun, AT&T Corp, NECI.
 *  Includes parts of TL3:
 *    Copyright (C) 1987-1999 Leon Bottou and Neuristique.
 *  Includes selected parts of SN3.2:
 *    Copyright (C) 1991-2001 AT&T Corp.
 * 
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as 
 *  published by the Free Software Foundation; either version 2.1 of the
 *  License, or (at your option) any later version.
 * 
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 * 
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, 
 *  MA  02110-1301  USA
 *
 ***********************************************************************/

/***********************************************************************
 *  This file is derived from SN-2.8
 *    Copyright (C) 1987-1999 Neuristique s.a.
 ***********************************************************************/

/***********************************************************************
 * $Id: network.c,v 1.3 2005/08/04 18:20:42 leonb Exp $
 **********************************************************************/



/************************************************************************
*                                                                       *
*       network.c: ensemble de routines permettant de reserver de       *
*           la place memoire pour des reseaux.., de les charger         *
*           les construire, les effacer etc...                          *
*                                               L.Y.B. 5/87             *
*             modif pour reseaux itratifs Yann le Cun 10/87             *
*                                                                       *
************************************************************************/

#include "defn.h"

static int *netconvert = NIL;   /* Tableau d'entier destines a stocker  */
                                /* les conversions a effectuer au cours */
                                /* des sauvegardes et relectures ..     */

/* zero_reseau
   routines d'initialisation et de remise a zero pour main.c
*/

static void
zero_reseau(void)
{
  if (neurbase!=NIL) {
#ifndef NONEURTYPE
    int i;
    for (i=0; i<neurmax; i++)
      set_neurtype(neuraddress[i], NULL, NULL);
#endif
    free((char *)neurbase);
    neurbase = NIL;
    neurnombre = neurmax = 0;
  }
  if (synbase!=NIL) {
    free((char *)synbase);
    synbase = NIL;
    synnombre = synmax = 0;
  }
#ifdef ITERATIVE
  if (weightbase!=NIL) {
    weightbase = NIL;
    weightnombre = weightmax = 0;
  }
  if (w_matrix != NIL) {
    unprotect(w_matrix);
    if (w_matrix_var)
      var_SET(w_matrix_var,NIL);
  }
#endif
  if (neuraddress!=NIL)
    free((char *)neuraddress);
  if (netconvert != NIL)
    free((char *)netconvert);
}


/* set_vars
   sets the lisp variables
   Nnum,Snum,Wnum,Nmax,Smax,Nmax
*/
void 
set_vars(void)
{
  at *p;
  
  p = NEW_NUMBER(neurnombre);
  var_SET(var_Nnum,p);
  p = NEW_NUMBER(synnombre);
  var_SET(var_Snum,p);
  p = NEW_NUMBER(neurmax);
  var_SET(var_Nmax,p);
  p = NEW_NUMBER(synmax);
  var_SET(var_Smax,p);
#ifdef ITERATIVE
  p = NEW_NUMBER(weightnombre);
  var_SET(var_Wnum,p);
  p = NEW_NUMBER(weightmax);
  var_SET(var_Wmax,p);
#endif
}


/* ALLOC_NET  (C)  xALLOC_NET (lisp)
   Allocation d'un espace reseau pour n1 neurones et n2 synapses
*/


#ifdef ITERATIVE
static int 
netalloc(int n1, int n2, int n3)
#else
static int 
netalloc(int n1, int n2)
#endif
{
  int c;
  neurone *n;
  typedef neurone *neuronePtr;
  
#ifdef ITERATIVE
  if (n3>n2)
    error(NIL,"you never need more weights than connections",NIL);
#endif
  zero_reseau();
  neurbase = (neurone *)malloc(n1 * sizeof(neurone));
  neurmax = n1;
  synbase = (synapse *)malloc(n2 * sizeof(synapse));
  synmax  = n2;
#ifdef ITERATIVE
  { 
    int dim[2];
    struct index *arr;
    dim[0] = n3;
    dim[1] = sizeof(weight) / sizeof(flt);
    w_matrix = F_matrix(2,dim);
    protect(w_matrix);
    if (w_matrix_var)
      var_SET(w_matrix_var,w_matrix);
    arr = Mptr(w_matrix);
    weightbase = arr->st->srg.data;
    weightmax = n3;
  }
#endif
  neuraddress = (neurone **)malloc(n1 * sizeof( neuronePtr ));
  netconvert = (int *)malloc(n1 * sizeof(int));
  if ( neurbase==NIL || synbase==NIL || 
      neuraddress==NIL || netconvert ==NIL ) {
    zero_reseau();
    error(NIL,"not enough memory",NIL);
  };
  memset(neurbase, 0, n1*sizeof(neurone));
  memset(synbase, 0, n2*sizeof(synapse));
  neurnombre = 1;
  neurbase[0].Nval=fptoF(1.0);    /* neurone 0 = seuil */
  for (c=0, n=neurbase ; c<neurmax; c++, n++)
    neuraddress[c]=n;
  set_vars();
#ifdef ITERATIVE
  return ( n3*sizeof(weight) + n2*sizeof(synapse) + n1*sizeof(neurone) );
#else
  return ( n2*sizeof(synapse) + n1*sizeof(neurone) );
#endif
}


#ifdef ITERATIVE
DX(xalloc_net)
{
  int n1,n2,n3;
  
  ALL_ARGS_EVAL;
  if (arg_number==2) {
    n1 = AINTEGER(1);
    n2=n3 = AINTEGER(2);
  } else {
    ARG_NUMBER(3);
    n1 = AINTEGER(1);
    n2 = AINTEGER(2);
    n3 = AINTEGER(3);
  }
  if (n1<0 || n2<0 || n3<0)
    error(NIL,"illegal negative value",NIL);
  return NEW_NUMBER(netalloc(n1,n2,n3));
}
#else
DX(xalloc_net)
{
  int n1,n2;
  ARG_NUMBER(2);
  ALL_ARGS_EVAL;
  n1 = AINTEGER(1);
  n2 = AINTEGER(2);
  if (n1<0 || n2<0)
    error(NIL,"illegal negative value",NIL);
  return NEW_NUMBER(netalloc(n1,n2));
}
#endif


/* ( newneurons N )
   retourne une liste de N neurones inutilises
*/

DX(xnewneurons)
{
  int n;
  at *ans, **where;
  int neur;
  ARG_NUMBER(1);
  ARG_EVAL(1);
  n = AINTEGER(1);
  if (n<=0)
    error(NIL,"illegal negative value",NIL);
  ans=NIL;
  where = &ans;
  neur=neurnombre;
  while (n-- >0)
    {
      if (neur>=neurmax)
	error(NIL,"too many neurons",NIL);
      *where = new_cons( NEW_NUMBER(neur++),NIL);
      where = &Cdr(*where));
    };
  neurnombre=neur;
  set_vars();
  return ans;
}



/* SET_NEURTYPE_REGULAR
 * set neuron type to default value
 */

#ifndef NONEURTYPE
void 
set_neurtype(neurone *n, neurtype *newtype, void *newparms)
{
  if (n->type && n->type->detach_parms)
    if (newparms != n->parms)
      (*n->type->detach_parms)(n);
  n->type = newtype;
  n->parms = newparms;
}

static void
set_neurtype_regular(neurone *n)
{
  set_neurtype(n, NULL, NULL);
}

DX(xset_neurtype_regular)
{
  ARG_NUMBER(1);
  ARG_EVAL(1);
  mapneur(ALIST(1), set_neurtype_regular);
  return NIL;
}

DX(xget_neurtype)
{
  int nindex;
  neurone *n;
  ARG_NUMBER(1);
  ARG_EVAL(1);
  nindex = AINTEGER(1);
  if (nindex < 0 || nindex>=neurnombre)
    error(NIL, "illegal neuron index", APOINTER(1));
  n = neuraddress[nindex];
  if (!n->type)
    return new_safe_string("regular:0");
  sprintf(string_buffer, "%s:%lx", n->type->name, (long)(n->parms));
  return new_string(string_buffer);
}
#endif




/* CLEAR_NET
 * Remise a zero du contenu de l'espace de neurones
 */
DX(xclear_net)
{
  int c;
  neurone *n;
  ARG_NUMBER(0);
  synnombre=0;
  neurnombre = (neurmax>0) ? 1 : 0;
#ifdef ITERATIVE
  weightnombre=0;
#endif
  memset(neurbase, 0, neurmax*sizeof(neurone));
  memset(synbase, 0, synmax*sizeof(synapse));
#ifdef ITERATIVE
  memset(weightbase, 0, weightmax*sizeof(weight));
#endif
  if (neurmax)
    neurbase[0].Nval= Flt1;
  for (c=0, n=neurbase ; c<neurmax; c++, n++)
    neuraddress[c]=n;
  set_vars();
#ifdef ITERATIVE
  return NEW_NUMBER( weightmax*sizeof(weight) + synmax*sizeof(synapse)
		    + neurmax*sizeof(neurone) );
#else
  return NEW_NUMBER ( synmax*sizeof(synapse) + neurmax*sizeof(neurone) );
#endif
}


/* Routine de connexion de deux neurones: cree une nouvelle synapse.
   syntaxe (connect N1 N2) ou (connect N1 N2 VAL)
    VAL par defaut: random()-0.5
*/

#ifdef ITERATIVE

/* connection: version reseaux iteratif */

synapse *
connection(int amont, int aval, float val, float eps)
{
  neurone *am, *av;
  synapse *s;
  if ( searchsynapse(amont,aval) != NIL )
    error(NIL,"this connection exists already",NIL);
  if ( synnombre>=synmax )
    error(NIL,"too many connections",NIL);
  if ( weightnombre>=weightmax )
    error(NIL,"too many weights",NIL);
  s= &synbase[synnombre++];
  s->w= &weightbase[weightnombre++];
  am= &neurbase[amont];
  av= &neurbase[aval];
#ifdef SYNEPSILON
  s->Sepsilon=eps;
#endif
  s->Sval=val;
  s->Sdelta=Flt0;
  s->Sacc=Flt0;
  s->Scounter=Flt1;
  s->Namont=am;
  s->Naval=av;
  s->NSamont=am->FSaval;
  am->FSaval=s;
  s->NSaval=av->FSamont;
  av->FSamont=s;
  return s;
}

#else

/* connection: version reseaux non iteratifs */
synapse *
connection(int amont, int aval, float val, float eps)
{
  neurone *am, *av;
  synapse *s;
  if ( searchsynapse(amont,aval) != NIL )
    error(NIL,"this connection exists already",NIL);
  if ( synnombre>=synmax )
    error(NIL,"too many connections",NIL);
  s= &synbase[synnombre++];
  am= &neurbase[amont];
  av= &neurbase[aval];
#ifdef SYNEPSILON
  s->Sepsilon=eps;
#endif
  s->Sval=val;
  s->Sdelta=Flt0;
  s->Namont=am;
  s->Naval=av;
  s->NSamont=am->FSaval;
  am->FSaval=s;
  s->NSaval=av->FSamont;
  av->FSamont=s;
  return s;
}

#endif

static void 
testlist(at *q)
{
  at *p;
  for(p=q; CONSP(p); p=Cdr(p)) {
    if (! NUMBERP(Car(p)) )
      error(NIL,"not a cell number", Car(p));
    else if (Number(Car(p)) < 0 || Number(Car(p)) > neurnombre)
      error(NIL,"illegal cell number", Car(p));
  }
  if (p)
    error(NIL,"illegal cell list",q);
}

DX(xconnect)
{
  at *aml,*avl,*am2,*av2;
  extern at *flatten(at *);
  int amont,aval;
  flt val,eps;
  
  ALL_ARGS_EVAL;
  if (arg_number == 4) {
    amont = AINTEGER(1);
    aval = AINTEGER(2);
    val = AFLT(3);
    eps = AFLT(4);
    connection(amont,aval,val,eps);
  } else if (arg_number == 3) {
    amont = AINTEGER(1);
    aval = AINTEGER(2);
    val = AFLT(3);
    eps = Flt0;
    connection(amont,aval,val,eps);
  } else {
    ARG_NUMBER(2);
    /* Full Connect effect !! */
    aml = flatten(APOINTER(1));
    avl = flatten(APOINTER(2));
    testlist(aml);
    testlist(avl);
    for( am2=aml; am2; am2=Cdr(am2) )
      for( av2=avl; av2; av2=Cdr(av2) )
	connection((int)Number(Car(am2)), (int)Number(Car(av2)),
		   Flt0, Flt0 );
  }
  set_vars();
  return NIL;
}

#ifdef ITERATIVE

/* duplicate a connection, the connection created by this function
   use the same weight as a given, previously created connection.
   if a synapse value is specified, the average of the previous and
   the new values is used. The previous value is weighted by the
   number of connections already using the weight */

synapse *
dup_connection(int fromAmont, int fromAval, int amont, int aval, 
	       int Flag, float val, float eps)
{
  neurone *am, *av;
  synapse *s, *froms;
  if (searchsynapse(amont,aval) != NIL )
    error(NIL,"the target connection exists already",NIL);
  if ((froms=searchsynapse(fromAmont,fromAval)) == NIL )
    error(NIL,"the source connection does'nt exist",NIL);
  if ( synnombre>=synmax )
    error(NIL,"too many connections",NIL);
  s= &synbase[synnombre++];
  s->w= froms->w;
  am= &neurbase[amont];
  av= &neurbase[aval];
#ifdef SYNEPSILON
  s->Sepsilon=eps;
#endif
  if (Flag)
    s->Sval=(val + s->Scounter * s->Sval )/( s->Scounter +1 );
  s->Sdelta=fptoF(0.0);
  s->Sacc=fptoF(0.0);
  (s->Scounter)++;
  s->Namont=am;
  s->Naval=av;
  s->NSamont=am->FSaval;
  am->FSaval=s;
  s->NSaval=av->FSamont;
  av->FSamont=s;
  return s;
}

DX(xdup_connection)
{
  at *aml,*avl,*sml,*svl;
  at *am2,*av2,*sm2,*sv2;
  int amont,aval,smont,sval;
  flt val,eps;

  ALL_ARGS_EVAL;
  if (arg_number == 6) {
    smont = AINTEGER(1);
    sval = AINTEGER(2);
    amont = AINTEGER(3);
    aval = AINTEGER(4);
    val = AFLT(5);
    eps = AFLT(6);
    dup_connection(smont,sval,amont,aval,1,val,eps);
  } else if (arg_number == 5) {
    smont = AINTEGER(1);
    sval = AINTEGER(2);
    amont = AINTEGER(3);
    aval = AINTEGER(4);
    val = AFLT(5);
    eps = Flt0;
    dup_connection(smont,sval,amont,aval,1,val,eps);
  } else {
    ARG_NUMBER(4);
    /* Full Connect effect !! */
    sml = flatten(APOINTER(1));
    svl = flatten(APOINTER(2));
    aml = flatten(APOINTER(3));
    avl = flatten(APOINTER(4));
    testlist(aml);
    testlist(avl);
    testlist(sml);
    testlist(svl);
    if ( length(aml) != length(sml) || length(avl) != length(svl) )
      error(NIL,"source and target listes should have the same length",NIL);
    for( am2=aml,sm2=sml; am2 && sm2; am2=Cdr(am2), sm2=Cdr(sm2) )
      for( av2=avl,sv2=svl; av2 && sv2; av2=Cdr(av2),sv2=Cdr(sv2) )
	dup_connection( (int)Number(Car(sm2)), (int)Number(Car(sv2)),
                        (int)Number(Car(am2)), (int)Number(Car(av2)),
		       0,Flt0, Flt0 );
  }
  set_vars();
  return NIL;
}

#endif




/* CUT_CONNECTION
 * (cut-connection <fromcell> <tocell>)
 * Removes a connection from the update lists.
 */


synapse *
cut_connection(int amont, int aval)
{
  synapse *target, *current, **s;
  if (aval<0 || amont<0 || aval>=neurnombre || amont>=neurnombre )
    error(NIL,"Illegal neuron number",NIL);
  target = searchsynapse(amont,aval);
  if (!target)
    error(NIL,"Synapse not found",NIL);
  s = &neurbase[amont].FSaval;
  current = 0;
  while (*s) 
    {
      current = *s;
      if (current==target) {
	*s = current->NSamont;
	break;
      }
      s = &( current->NSamont );
    }
  if (current != target)
    error(NIL,"Internal error: incoherent data structure (i)",NIL);
  s = &neurbase[aval].FSamont;
  while (*s) {
    current = *s;
    if (current==target) {
      *s = current->NSaval;
      break;
    }
    s = &( current->NSaval );
  }
  if (current != target)
    error(NIL,"Internal error: incoherent data structure (ii)",NIL);
#ifdef ITERATIVE
  target->w->Wcounter -= Fone;
  if (target->w->Wcounter == Fzero) {
    target->w->Wval = Fzero;
    target->w->Wacc = Fzero;
  }
#else
  target->Sval = Fzero;
#endif
  return target;
}


DX(xcut_connection)
{
  ARG_NUMBER(2);
  ALL_ARGS_EVAL;
  cut_connection(AINTEGER(1),AINTEGER(2));
  return NIL;
}



/* amont. aval.
   Returns the list of upstream/downstream neurons to a neuron
*/

DX(xamont)
{
  at *a;
  neurone *n;
  synapse *s;
  int i;
  a=NIL;
  ARG_NUMBER(1);
  ARG_EVAL(1);
  i = AINTEGER(1);
  if ( i<0 || i>=neurnombre )
    error(NIL,"illegal cell number",APOINTER(1));
  n= &(neurbase[i]);
  for ( s=n->FSamont; s!=NIL; s=s->NSaval )
    {
      a = cons(NEW_NUMBER(s->Namont - neurbase),a);
      if (s->Naval != n )
	error("amont","internal error -8",NIL);
    }
  return a;
}


DX(xaval)
{
  at *a;
  neurone *n;
  synapse *s;
  int i;
  a=NIL;
  ARG_NUMBER(1);
  ARG_EVAL(1);
  i = AINTEGER(1);
  if ( i<0 || i>=neurnombre )
    error(NIL,"illegal cell number",APOINTER(1));
  n = &(neurbase[i]);
  for ( s=n->FSaval; s!=NIL; s=s->NSamont )
    {
      a = cons(NEW_NUMBER(s->Naval - neurbase),a);
      if (s->Namont != n )
	error("aval","internal error -8",NIL);
    }
  return a;
}



/*  ==============================================================
    Weights saving and loading function
    ==============================================================   */


/* magic number */
#define NMAGIC  0x2d4f1c93
/* magic number for ascii files */
#define NAMAGIC 0x2e574549
/* old magic number for synepsilon nets */
#define NSMAGIC 0x2d4f1c91

struct Nheader {
  int magic;
  int nombre;
  int age;
} Nheader;

struct S2record {
  int  pre,post;
  flt  val;
  flt  epsi;
} S2record;
                    
struct Srecord {
  int  pre,post;
  flt  val;
} Srecord;

static int indice;

static void 
clearconvert(void)
{       
  int i;
  ifn (netconvert)
    error(NIL,"you should perform (alloc-net xx xx)",NIL);
  for (i=1; i<neurnombre; i++)
    netconvert[i]= -1;
  netconvert[0]=0;
}

static void 
convertsave(neurone *n)  /* pour mapneur */
{
  int i;
  i=n-neurbase;
  if ( netconvert[i]<0 )
    netconvert[i]= (++indice);
  else
    error(NIL,"cell referenced twice in 'save-net' listes",NEW_NUMBER(i));
}

static void 
convertload(neurone *n)  /* pour mapneur */
{
  netconvert[++indice] = n-neurbase;
}



DX(xsave_net)
{
  char *s;
  FILE *f;
  synapse *ss;
  int i,npre,npost;
  if (arg_number < 1)
    error(NIL,"filename string expected",NIL);
  ALL_ARGS_EVAL;
  s = ASTRING(1);
  if (arg_number > 1)
    {       
      clearconvert();
      indice=0;
      for (i=2; i<= arg_number; i++)
	mapneur(ALIST(i),convertsave);
    }
  else
    {       
      for (i=0;i<neurnombre;i++)
	netconvert[i]=i;
      indice= --i;
    }
  f = open_write(s,"SNWT@wei");
  Nheader.magic=NMAGIC;
  Nheader.age= readage();
  Nheader.nombre=indice;
  if ( fwrite((char *)&Nheader,(unsigned)sizeof(Nheader),(int)1,f) == 0 )
    goto wproblem;
  
  for ( i=0,ss=synbase;  i<synnombre;  i++,ss++ ) {
    npre  = netconvert[ ss->Namont -neurbase];
    npost = netconvert[ ss->Naval -neurbase];
    
    if (npre>=0 && npost>=0 ) {
      Srecord.pre  = npre;
      Srecord.post = npost;
      Srecord.val  = ss->Sval;
      if ( fwrite((char *)&Srecord,sizeof(Srecord),1,f) == 0)
	goto wproblem;
    }
  }
  file_close(f);
  LOCK(APOINTER(1));
  return APOINTER(1);
  
 wproblem:
  file_close(f);
  test_file_error(NIL);
  error(NIL,"can't write for an unknown reason",NIL);
}
     

DX(xsave_ascii_net)
{
  char *s;
  FILE *f;
  synapse *ss;
  int i,npre,npost;
  if (arg_number < 1)
    error(NIL,"filename string expected",NIL);
  
  ALL_ARGS_EVAL;
  s = ASTRING(1);
  if (arg_number > 1)
    {       
      clearconvert();
      indice=0;
      for (i=2; i<= arg_number; i++)
	mapneur(ALIST(i),convertsave);
    }
  else
    {       
      for (i=0;i<neurnombre;i++)
	netconvert[i]=i;
      indice= --i;
    }
  f = open_write(s,"wei");
  Nheader.age= readage();
  Nheader.nombre=indice;
  fprintf(f,".WEI %d %d\n", Nheader.nombre, Nheader.age );
  if (ferror(f))
    goto wproblem;
  for ( i=0,ss=synbase;  i<synnombre;  i++,ss++ ) {
    npre  = netconvert[ ss->Namont -neurbase];
    npost = netconvert[ ss->Naval -neurbase];
    if (npre>=0 && npost>=0 ) {
      fprintf( f,"%4d %4d %10.7f\n",
	      npre,npost,Ftofp(ss->Sval) );
      if (ferror(f))
	goto wproblem;
    }
  }
  file_close(f);
  LOCK(APOINTER(1));
  return APOINTER(1);
  
wproblem:
  file_close(f);
  test_file_error(NIL);
  error(NIL,"can't write for an unknown reason",NIL);
}





DX(xmerge_net)
{
  at *q;
  char *s;
  FILE *f;
  synapse *ss;
  int i,npre,npost;  
  int a,b;
  double x;
  
  if (arg_number < 1)
    error(NIL,"filename string expected",NIL);
  ALL_ARGS_EVAL;
  s = ASTRING(1);
	
  if (arg_number == 1)
    {                       /* pas de listes */
      f=open_read(s,"SNWT@wei");
      if (fread((char *)&Nheader,sizeof(Nheader),1,f) == 0)
	goto rproblem;
      
      switch (Nheader.magic)
	{
	case NSMAGIC:
	  print_string("WARNING: this file has been saved with an old format\n");
	  break;
	case NMAGIC:
	  break;
	case NAMAGIC:
	  file_close(f);
	  f=open_read(s,"wei");
	  if( fscanf(f,".WEI %d %d",&a,&b) != 2 )
	    goto rproblem; 
	  Nheader.nombre = a; 
	  Nheader.age = b;
	  break;
	default:              
	  file_close(f);
	  return NIL;
	}
      file_close(f);
      return NEW_NUMBER(Nheader.nombre);
    };
  
  clearconvert();
  indice=0;
  for (i=2; i<= arg_number; i++)
    mapneur(ALIST(i),convertload);
  
  f=open_read(s,"SNWT@wei");
  if (fread((char *)&Nheader,sizeof(Nheader),1,f) == 0)
    goto rproblem;
  
  switch (Nheader.magic)
    {
    case NSMAGIC:
      print_string("WARNING: this file has been saved with an old format\n");
      while( fread(&S2record,sizeof(S2record),1,f) != 0) {
	npre=netconvert[S2record.pre];
	npost=netconvert[S2record.post];
	ss=searchsynapse( npre,npost );
	if (ss!=NIL) {
	  ss->Sval = S2record.val;
#ifdef SYNEPSILON
	  ss->Sepsilon = S2record.epsi;
#endif
	} else
	  goto noconnect;
      }
      break;
      
    case NMAGIC:
      while(fread((char *)&Srecord,sizeof(Srecord),1,f) != 0) {
	npre=netconvert[Srecord.pre];
	npost=netconvert[Srecord.post];
	ss=searchsynapse( npre,npost );
	if (ss!=NIL)
	  ss->Sval=Srecord.val;
	else
	  goto noconnect;
      }
      break;
      
    case NAMAGIC:
      rewind(f);
      if( fscanf(f,".WEI %d %d",&a,&b) != 2 )
	goto rproblem; 
      Nheader.nombre = a; 
      Nheader.age = b;
      while( fscanf(f," %d %d %lf ",&a,&b,&x) == 3 ) {
	npre=netconvert[a];
	npost=netconvert[b];
	ss=searchsynapse( npre,npost );
	if (ss!=NIL)
	  ss->Sval= fptoF(x);
	else
	  goto noconnect;
      }
      break;
      
    default:              
      goto badmagic;
    }
  
  test_file_error(f);
  file_close(f);
  q = NEW_NUMBER(Nheader.age);
  var_SET(var_age,q);
  set_vars();
  return new_string(s);
  
 rproblem:
  file_close(f);
  test_file_error(NIL);
  error(NIL,"can't read for an unknown reason",NIL);
 badmagic:
  file_close(f);
  error(NIL,"this is probably not a weight file",NIL);
 noconnect:
  file_close(f);
  error(NIL,"you should create the connections first",NIL);
}





/* --------- MATRIX EXCHANGE OPS --------- */


DX(xweight_to_matrix)
{
  at *p;
  int i;
  int n;
  struct index *arr;

#ifdef ITERATIVE
  n = weightnombre;
#else
  n = synnombre;
#endif
  if (n<1)
    error(NIL,"No network created",NIL);

  if (arg_number==0) {
    p = F_matrix(1,&n);
  } else {
    ARG_NUMBER(1);
    ARG_EVAL(1);
    p = APOINTER(1);
    LOCK(p);
  }

  arr = easy_index_check(p, 1, &n);
  for (i=0;i<n;i++) {
#ifdef ITERATIVE
    flt f = weightbase[i].Wval;
#else
    flt f = synbase[i].Sval;
#endif
    easy_index_set(arr, &i, (real) f);
  }
  return p;
}

DX(xmatrix_to_weight)
{
  at *p;
  int n;
  int i;
  struct index *arr;

  ARG_NUMBER(1);
  ARG_EVAL(1);
  p = APOINTER(1);
#ifdef ITERATIVE
  n = weightnombre;
#else
  n = synnombre;
#endif
  if (n<1)
    error(NIL,"No network created",NIL);

  arr = easy_index_check(p, 1, &n);
  for (i=0;i<n;i++) {
    flt f = (flt) easy_index_get(arr, &i);
#ifdef ITERATIVE
    weightbase[i].Wval = f;
#else
    synbase[i].Sval = f;
#endif
  }
  return NIL;
}


#ifdef ITERATIVE

DX(xacc_to_matrix)
{
  at *p;
  int i;
  int n;
  struct index *arr;

  n = weightnombre;
  if (n<1)
    error(NIL,"No network created",NIL);
  if (arg_number==0) {
    p = F_matrix(1,&n);
  } else {
    ARG_NUMBER(1);
    ARG_EVAL(1);
    p = APOINTER(1);
    LOCK(p);
  }
  arr = easy_index_check(p, 1, &n);
  for (i=0;i<n;i++) 
    easy_index_set(arr, &i, (real) weightbase[i].Wacc);
  return p;
}

DX(xmatrix_to_acc)
{
  at *p;
  int n;
  int i;
  struct index *arr;

  ARG_NUMBER(1);
  ARG_EVAL(1);
  p = APOINTER(1);
  n = weightnombre;
  if (n<1)
    error(NIL,"No network created",NIL);
  arr = easy_index_check(p, 1, &n);
  for (i=0;i<n;i++) 
    weightbase[i].Wacc =  (flt) easy_index_get(arr, &i);
  return NIL;
}

DX(xdelta_to_matrix)
{
  at *p;
  int i;
  int n;
  struct index *arr;

  n = weightnombre;
  if (n<1)
    error(NIL,"No network created",NIL);
  if (arg_number==0) {
    p = F_matrix(1,&n);
  } else {
    ARG_NUMBER(1);
    ARG_EVAL(1);
    p = APOINTER(1);
    LOCK(p);
  }
  arr = easy_index_check(p, 1, &n);
  for (i=0;i<n;i++) 
    easy_index_set(arr, &i, (real) weightbase[i].Wdelta);
  return p;
}

DX(xmatrix_to_delta)
{
  at *p;
  int n;
  int i;
  struct index *arr;

  ARG_NUMBER(1);
  ARG_EVAL(1);
  p = APOINTER(1);
  n = weightnombre;
  if (n<1)
    error(NIL,"No network created",NIL);
  arr = easy_index_check(p, 1, &n);
  for (i=0;i<n;i++) 
    weightbase[i].Wdelta =  (flt) easy_index_get(arr, &i);
  return NIL;
}

#ifdef NEWTON

DX(xhess_to_matrix)
{
  at *p;
  int i;
  int n;
  struct index *arr;

  n = weightnombre;
  if (n<1)
    error(NIL,"No network created",NIL);
  if (arg_number==0) {
    p = F_matrix(1,&n);
  } else {
    ARG_NUMBER(1);
    ARG_EVAL(1);
    p = APOINTER(1);
    LOCK(p);
  }
  arr = easy_index_check(p, 1, &n);
  for (i=0;i<n;i++) 
    easy_index_set(arr, &i, (real) weightbase[i].Whess);
  return p;
}

DX(xmatrix_to_hess)
{
  at *p;
  int n;
  int i;
  struct index *arr;

  ARG_NUMBER(1);
  ARG_EVAL(1);
  p = APOINTER(1);
  n = weightnombre;
  if (n<1)
    error(NIL,"No network created",NIL);
  arr = easy_index_check(p, 1, &n);
  for (i=0;i<n;i++) 
    weightbase[i].Whess =  (flt) easy_index_get(arr, &i);
  return NIL;
}

#endif /* NEWTON */
#endif /* ITERATIVE */


/* --------- INITIALISATION CODE --------- */

void 
init_network(void)
{
  /* Network Creation */
  dx_define("alloc-net",xalloc_net);
  dx_define("newneurons",xnewneurons);
#ifndef NONEURTYPE
  dx_define("set-neurtype-regular", xset_neurtype_regular);
  dx_define("get-neurtype", xget_neurtype);
#endif
  dx_define("clear_net",xclear_net);
  dx_define("connect",xconnect);
  dx_define("cut-connection", xcut_connection);
#ifdef ITERATIVE
  dx_define("dup-connection",xdup_connection);
#ifdef WMATRIX
  w_matrix_var = var_define("weight_matrix");
#endif
#endif
  
  /* Topology functions */
  dx_define("amont",xamont);
  dx_define("aval",xaval);

  /* File functions */
  dx_define("save-net/merge",xsave_net);
  dx_define("save-ascii-net/merge",xsave_ascii_net);
  dx_define("merge-net",xmerge_net);
  
  /* Network to matrix transferts */
  dx_define("weight-to-matrix",xweight_to_matrix);
  dx_define("matrix-to-weight",xmatrix_to_weight);
#ifdef ITERATIVE
  dx_define("acc-to-matrix",xacc_to_matrix);
  dx_define("matrix-to-acc",xmatrix_to_acc);
  dx_define("delta-to-matrix",xdelta_to_matrix);
  dx_define("matrix-to-delta",xmatrix_to_delta);
#ifdef NEWTON
  dx_define("hess-to-matrix",xhess_to_matrix);
  dx_define("matrix-to-hess",xmatrix_to_hess);
#endif
#endif
}
