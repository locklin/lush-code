/*   
 * This file is part of the subset of the SN3.2 Lisp 
 * interpreter/compiler and libraries that are not owned by Neuristique S.A.
 * (i.e. the pieces owned by either AT&T, Leon Bottou, or Yann LeCun).
 * This code is not functional as is, but is intended as a 
 * repository of code to be ported to Lush.
 *
 *
 *   SN3.2
 *   Copyright (C) 1987-2001 
 *   Leon Bottou, Yann Le Cun, AT&T Corp.
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
 *
 */

/* gdbm_driver.c

	Yoshua Bengio 12/92

	``inspired'' or translated from DataBase.c (c++) by B. Boser

	Compilation needs -I../gdbm
*/

#include "define.h"
#include "header.h"

#include "gdbm.h"
#include <math.h>

/*#define KEYBUF*/
#ifdef KEYBUF
#define KEY_BUFFER_SIZE 1024
char KeyBuffer[KEY_BUFFER_SIZE];
int KeyBufferSize=0;
#endif

typedef struct DATABASE {
	GDBM_FILE db;
	char *name;
	bool closed;
	bool writeable;
	at *data_cache;
	datum key_cache;
} DataBase;

extern DataBase *new_db();
extern void db_open(),db_close(), print_datum(), db_error(), db_err(), 
			db_create(), db_store(), db_delete(), db_reorganize();
extern datum db_encode_key(), db_encode_data();
extern at *db_decode_key(), *db_decode_data(), *db_fetch(), *db_firstkey(),
	*db_nextkey();

/*
//////////////////////////////////////////////////////////////////////////
// DataBase

class DataBase {
    GDBM_FILE db;
    String name;
    bool closed;
    bool writeable;
    CP key_cache, data_cache;             // 1 element cache
public:
    DataBase();                           // create a closed DataBase object
    void Open(const char*, bool write);   // open a DataBase for reading (writ)
    void Create(const char*);             // create new DataBase
    void Close();                         // update access perm.
    void Store(CP key, CP data);
    CP Fetch(CP key, CP def = Nil);       // result ref'd
    void Delete(CP key);
    CP FirstKey();                        // result ref'd
    CP NextKey(CP key);                   // result ref'd
    void Reorganize();                    // prune unused space after delete
    const char* Name() const { return name; }
    bool Protected() const { return !writeable; }
private:
    void err();                           // print out gdbm_errno
    void print_datum(datum);              // for debugging
    datum encode_key(CP key);             // Lisp rep. vs. binary rep
    CP decode_key(datum key);
    datum encode_data(CP data);
    CP decode_data(datum data);
};  // DataBase
*/

/*
DataBase::DataBase() {
    writeable = closed = true;  name = "closed";  db = 0;
    key_cache = data_cache = Nil;
};  // constructor
*/

DataBase *new_db()
{
	DataBase *newdb = (DataBase *)malloc(sizeof(DataBase));
	newdb->writeable = newdb->closed = TRUE;
	newdb->name= "closed";
	newdb->db = NIL;
	newdb->data_cache = NIL;
	newdb->key_cache.dsize = 0;
	newdb->key_cache.dptr = NIL;
	return(newdb);
}

/*
void DataBase::print_datum(datum d) {
    fprintf(Stdout, "datum: dsize = %d,  dptr = ", d.dsize);
    fprintf(Stdout, "0x%08x\nContents: '", d.dptr);
    for (int i=0;  i<d.dsize;  i++) {
        int c = d.dptr[i];
        if (c >= ' ' && c <= '~') fprintf(Stdout, "%c", d.dptr[i]);
                            else  fprintf(Stdout, " %d", d.dptr[i]);
    }
    fprintf(Stdout, "'\n");
};  // print_datum
*/

void db_print_datum(d)		/* datum = major gdbm struct (see ../gdbm/gdbm.h) */
datum d;
{
	int i,c;
	fprintf(stdout, "datum: dsize = %d, dptr = ", d.dsize);
	fprintf(stdout, "0x%08x\nContents: '", d.dptr);
   for (i=0;  i<d.dsize;  i++) {
        c = d.dptr[i];
        if (c >= ' ' && c <= '~') fprintf(stdout, "%c", d.dptr[i]);
                            else  fprintf(stdout, " %d", d.dptr[i]);
   }
   fprintf(stdout, "'\n");
}

/*
static void db_error(const char *msg) {
    error('E', "database: %s", msg);
};  // db_error
*/

void db_error(msg)
char *msg;
{
	error("database: ", msg, NIL);
}

extern gdbm_error gdbm_errno;

/*
void DataBase::err() {
    static const char* msg[] = {
                "nor",
		"malloc",
		"block size",
		"file open",
		"file write",
		"file seek",
		"file read error",
		"bad magic number",
		"empty database",
		"can't read",
	        "can't write",
		"reader can't recover",
		"reader can't delete",
		"reader can't store",
		"reader can't reorganize",
		"unknown update",
		"item not found",
		"reorganize failed",
		"cannot replace",
                "illegal data",
                "unknown" };
    int e = (int)gdbm_errno;
    if (e > GDBM_ILLEGAL_DATA || e < 0) e = GDBM_ILLEGAL_DATA + 1;
    if (e != GDBM_NO_ERROR)
        error('E', "database error: %s", msg[e]);
};  // err
*/

void db_err() {
    static char* msg[] = {
      "nor",
		"malloc",
		"block size",
		"file open",
		"file write",
		"file seek",
		"file read error",
		"bad magic number",
		"empty database",
		"can't read",
	   "can't write",
		"reader can't recover",
		"reader can't delete",
		"reader can't store",
		"reader can't reorganize",
		"unknown update",
		"item not found",
		"reorganize failed",
		"cannot replace",
      "illegal data",
      "unknown" };
    int e = (int)gdbm_errno;
    if (e > GDBM_ILLEGAL_DATA || e < 0) e = GDBM_ILLEGAL_DATA + 1;
    if (e != GDBM_NO_ERROR)
        error("database error: ", msg[e], NIL);
}

/*
void DataBase::Create(const char *n) {
    if (!closed) Close();
    db = gdbm_open(n, 0, GDBM_WRCREAT, 0644, db_error);
    if (!db) err();  name = n;  closed = false;  writeable = true;
};  // Create
*/

void db_create(DaBa, n)
DataBase *DaBa;
char *n;
{
    if (!DaBa->closed) db_close(DaBa);
    DaBa->db = gdbm_open(n, 0, GDBM_WRCREAT, 0644, db_error);
    if (!DaBa->db) db_err();  
	 DaBa->name = malloc(1+strlen(n));
    strcpy(DaBa->name,n);
    DaBa->closed = FALSE;  DaBa->writeable = TRUE;
}

/*
void DataBase::Open(const char *n, bool w) {
    if (!closed) Close();
    File *f = fopen(n, "r", -1);
    if (!f) error('E', "database %s not found", n);
    db = gdbm_open(f->name(), 0, 
                           w ? GDBM_WRITER : GDBM_READER, 0644, db_error);
    fclose(f);
    if (!db) err();
    closed = false;  writeable = w;  name = n;
};  // Open
*/

void db_open(DaBa, n, w)
DataBase *DaBa;
char *n;
bool w;
{
    File *f;
    char *fname;
    if (!DaBa->closed) db_close(DaBa);
    fname = search_file(n, "db");
    if (!fname) error("database not found: ", n, NIL);
    DaBa->db = gdbm_open(fname, 0, 
                           w ? GDBM_WRITER : GDBM_READER, 0644, db_error);
    if (!DaBa->db) db_err();
    DaBa->closed = FALSE;  DaBa->writeable = w;  
	 DaBa->name = malloc(1+strlen(n));
    strcpy(DaBa->name,n);
}

/*
void DataBase::Close() {
    if (closed) return;
    gdbm_close(db);
    closed = true;  name = "closed";
    unref(key_cache);  unref(data_cache);  key_cache = data_cache = Nil;
};  // Close
*/

void db_close(DaBa)
DataBase *DaBa;
{
    if (DaBa->closed) return;
    gdbm_close(DaBa->db);
    DaBa->closed = TRUE;  DaBa->name = "closed";
    if (DaBa->key_cache.dptr) free(DaBa->key_cache.dptr);	/* should be the only pointer to this area */
    UNLOCK(DaBa->data_cache);  
    DaBa->data_cache = NIL;
}

/*
void DataBase::Store(CP k, CP d) {
    if (closed) error('E', "database %s is closed", name);
    if (!writeable) error('E', "database %s is protected", name);
    if (k == Nil) error('E', "database %s: () is not a valid key", name);
    datum key  = db_encode_key(k);
    datum data = db_encode_data(d);
    // update the cache
    unref(key_cache);  unref(data_cache);  
    key_cache = ref(k);  data_cache = ref(d);
    // update the database
    if (gdbm_store(db, key, data, GDBM_REPLACE) != 0) err();
};  // Store
*/

void db_store(DaBa, k, d)
DataBase *DaBa;
at *k, *d;
{
    at *key_sto;
    at *data_sto;
    datum key, data;
    if (DaBa->closed) error("database is closed: ", DaBa->name, NIL);
    if (!DaBa->writeable) error("database is protected: ", DaBa->name, NIL);
    if (k == NIL) error("() is not a valid key for database ", DaBa->name);
    key_sto = new_U8_storage();
    data_sto = new_U8_storage();
    key  = db_encode_key(k,key_sto);
    data = db_encode_data(d,data_sto);
    /* update the cache */
    if (DaBa->key_cache.dptr) free(DaBa->key_cache.dptr);
    DaBa->key_cache.dsize = key.dsize;
    DaBa->key_cache.dptr = key.dptr;
    UNLOCK(DaBa->data_cache);  
    LOCK(d);
	 DaBa->data_cache = d;
    /* update the database */
    if (gdbm_store(DaBa->db, key, data, GDBM_REPLACE) != 0) db_err();
    /* to prevent the UNLOCK(key_sto) from freeing the area pointed to by key_cache now */
    ((struct storage *)(key_sto->Object))->srg.data = 
          ((struct storage *)(key_sto->Object))->allinfo.sts_malloc.addr = NIL;
    ((struct storage *)(key_sto->Object))->srg.size = 0;
    UNLOCK(key_sto);
    UNLOCK(data_sto);
}


/*
CP DataBase::Fetch(CP k, CP def) {
    if (closed) error('E', "database %s is closed", name);
    // first check the cache
    if (key_cache->equal(k)) return ref(data_cache);
    // retrieve from database
    datum key  = encode_key(k);
    datum data = gdbm_fetch(db, key);
    // update cache if found
    if (data.dptr) {
        unref(key_cache);  unref(data_cache);
        key_cache = ref(k);  data_cache = decode_data(data);
        return ref(data_cache);
    }
    // return default if key doesn't exist
    return ref(def);
};  // Fetch
*/

at *db_fetch(DaBa, k)
DataBase *DaBa;
at *k;
{
    datum key, data;
    at *key_sto;
    if (DaBa->closed) error("database is closed: ", DaBa->name, NIL);
    /* convert the key */
	 key_sto = new_U8_storage();
    key  = db_encode_key(k,key_sto);
    /* first check the cache */
    if (DaBa->key_cache.dptr && (DaBa->key_cache.dsize==key.dsize) &&
				(memcmp(DaBa->key_cache.dptr, key.dptr, key.dsize)==0))
					/* i.e., same key (when translated in "binary" format)*/
    {   LOCK(DaBa->data_cache);
        return(DaBa->data_cache);
    }
    /* retrieve from database */
    data = gdbm_fetch(DaBa->db, key);
    /* update cache if found */
    if (data.dptr) {
        UNLOCK(DaBa->data_cache);
		  DaBa->data_cache = db_decode_data(data);
		  LOCK(DaBa->data_cache);
    	  if (DaBa->key_cache.dptr) free(DaBa->key_cache.dptr);
        DaBa->key_cache.dptr = key.dptr;  
        DaBa->key_cache.dsize = key.dsize;  
        /* to prevent the UNLOCK from freeing the area pointed to by key_cache now */
        ((struct storage *)(key_sto->Object))->srg.data = ((struct storage *)(key_sto->Object))->allinfo.sts_malloc.addr = NIL;
        ((struct storage *)(key_sto->Object))->srg.size = 0;
    	  UNLOCK(key_sto);
		  return(DaBa->data_cache);
    }
    UNLOCK(key_sto);
    return(NIL);
}

#ifdef KEYBUF
at *db_next_fetch(DaBa)
DataBase *DaBa;
{
    datum key, data;
    if (DaBa->closed) error("database is closed: ", DaBa->name, NIL);
    if (KeyBufferSize==0)
      db_firstkey(DaBa);
    else
      db_nextkey(DaBa,NIL);

    /* retrieve from database */
    key.dptr  = KeyBuffer;
    key.dsize = KeyBufferSize;
    data = gdbm_fetch(DaBa->db, key);
    if (data.dptr)
    	return(db_decode_data(data));
    else return(NIL);
}
#endif

/*
void DataBase::Delete(CP k) {
    if (closed) error('E', "database %s is closed", name);
    datum key  = encode_key(k);
    gdbm_delete(db, key);
};  // Delete
*/

void db_delete(DaBa, k)
DataBase *DaBa;
at *k;
{
    datum key;
    at *key_sto;
    if (DaBa->closed) error("database is closed: ", DaBa->name, NIL);
    key_sto = new_U8_storage();
    key  = db_encode_key(k,key_sto);
    gdbm_delete(DaBa->db, key);
    UNLOCK(key_sto);
} 

/*
CP DataBase::FirstKey() {
    if (closed) error('E', "database %s is closed", name);
    datum key = gdbm_firstkey(db);
    return key.dptr ? db_decode_data(key) : Nil;
};  // FirstKey
*/

at *db_firstkey(DaBa)
DataBase *DaBa;
{
    datum key;
    if (DaBa->closed) error("database is closed: ", DaBa->name, NIL);
    key = gdbm_firstkey(DaBa->db);
#ifdef KEYBUF
    if (key.dsize<KEY_BUFFER_SIZE) {
       KeyBufferSize=key.dsize;
       memcpy(KeyBuffer,key.dptr,key.dsize);
    }
#endif
    return(key.dptr ? db_decode_data(key) : NIL);
}

/*
CP DataBase::NextKey(CP k) {
    if (closed) error('E', "database %s is closed", name);
    datum key  = encode_key(k);
    datum next = gdbm_nextkey(db, key);
    return next.dptr ? decode_key(next) : Nil;
};  // NextKey
*/

at *db_nextkey(DaBa, k)
DataBase *DaBa;
at *k;
{
    datum key, next;
    at *key_sto = new_U8_storage();
    if (DaBa->closed) error("database is closed: ", DaBa->name, NIL);
#ifdef KEYBUF
    if (key.dsize>0) {
		 key.dptr=KeyBuffer;
		 key.dsize=KeyBufferSize;
    }
    else 
#endif
		key  = db_encode_key(k,key_sto);
    next = gdbm_nextkey(DaBa->db, key);
#ifdef KEYBUF
    if (next.dptr)
    {	KeyBufferSize=next.dsize;
    	memcpy(KeyBuffer,next.dptr,next.dsize);
	 }
	 else KeyBufferSize=0;
#endif
    UNLOCK(key_sto);
    return(next.dptr ? db_decode_key(next) : NIL);
}

/*
void DataBase::Reorganize() {
    if (closed) error('E', "database %s is closed", name);
    gdbm_reorganize(db);
};  // Reorganize
*/

void db_reorganize(DaBa)
DataBase *DaBa;
{
    if (DaBa->closed) error("database is closed: ", DaBa->name, NIL);
    gdbm_reorganize(DaBa->db);
}

/*
datum DataBase::encode_key(CP key) {
    static String str = Stringsize(200); // was 100
    static File *f = 0;
    if (!f) f = new File(&str, "encode-key");  str = "";
    CP cp = key;
    if (cp->typ() == ConsT) {
        if (cp->GetCdr() != Nil) {
            do {
		CP car = cp->GetCar(cp);
		if (car->typ() == RealT) out_int(f, (int)car->GetReal());
		                    else out_cp(f, car);
	    } while (cp->typ() == ConsT);
            if (cp != Nil) {  // dotted pair
                str = "";  out_cp(f, key);
            }
        } else out_cp(f, key);
    } else out_cp(f, key);
    datum result;  
    result.dptr = (char*)((const char*)str);  result.dsize = str.length();
    return result;
};  // encode_key
*/

datum db_encode_key(key,sto)
at *key,*sto;
{
    File *f = pseudo_fopen(sto, "w");
    datum res;
    at *cp = key;
    if (cp && cp->ctype == XT_CONS) {
        if (cp->Cdr != NIL) {
            do {
		at *car = cp->Car;
		cp = cp->Cdr;
		if (car->ctype == XT_NUMBER && 
/*			((int)(car->Number)==irint(car->Number)))*/
			((int)(car->Number)==rint(car->Number)))
/*			bugged_boser_lisp_out_int(f,
						  irint(car->Number));*/
			bugged_boser_lisp_out_int(f, rint(car->Number));
		else out_at(f, car, NIL);
	    } while (cp && cp->ctype == XT_CONS);
            if (cp != NIL) {  /* dotted pair */
		FCLOSE(f); 
    		f = pseudo_fopen(sto, "w");
                out_at(f, key, NIL);
            }
        } else out_at(f, key, NIL);
    } else out_at(f, key, NIL);
    res.dptr = (char*)(((struct storage *)(sto->Object))->srg.data);  
    res.dsize = ((struct storage *)(sto->Object))->srg.size;
    FCLOSE(f);
    return(res);
}

/*
void buildlist(CP &head, CP &last, CP atom) {
    // atom must have refcount incremented
    CP tmp = NewConsCell(atom);
    if (head == Nil) {
   head = last = tmp;
    } else {
   last->SetCdr(tmp);
   last = tmp;
    }
};  // buildlist
*/

void buildlist(head, last, atom)
at **head, **last, *atom;
{
   at *tmp = cons(atom,NIL);
   if (*head == NIL)
	*head = *last = tmp;
   else {
	UNLOCK((*last)->Cdr);
	(*last)->Cdr = tmp;
	*last = tmp;
   }
}

/*
CP DataBase::decode_key(datum k) {
    String str(k.dptr, k.dsize);
    File f(&str, "decode-key");
    bool eof;
    CP key = Parse(&f, eof);
    if (!feof(&f)) {
        CP head = Nil,  last = Nil;
        buildlist(head, last, key);
        while (!feof(&f)) buildlist(head, last, Parse(&f, eof));
        key = head;
    }
    free(k.dptr);  return key;
};  // decode_key
*/

at *db_decode_key(k)
datum k;
{
    at *key, *at_sto = new_U8_storage();
    File *f;
    struct storage *sto= at_sto->Object;
    printf("WARNING: In db_decode_key: untested function\n");
    sto->srg.data = sto->allinfo.sts_malloc.addr = k.dptr;
    sto->srg.size = k.dsize;
    sto->srg.flags = STS_MALLOC;
    f = pseudo_fopen(at_sto, "r");
    key = in_at(f);
    if (!FEOF(f)) {
        at *head = NIL,  *last = NIL;
        buildlist(&head, &last, key);
        while (!FEOF(f)) buildlist(&head, &last, in_at(f));
        key = head;
    }
    sto->srg.data = sto->allinfo.sts_malloc.addr = NIL;
    sto->srg.size = 0;
    FCLOSE(f);
    UNLOCK(at_sto);
    return key;
}

/*
datum DataBase::encode_data(CP data) {
    static String str;
    static File *f = 0;
    if (!f) f = new File(&str, "encode-data");  str = "";
    out_cp(f, data);
    datum result;  
    result.dptr = (char*)((const char*)str);  result.dsize = str.length();
    return result;
};  // encode_data
*/

datum db_encode_data(data,sto)
at *data,*sto;
{
    File *f = pseudo_fopen(sto, "w");
    datum result;  
    out_at(f, data, NIL);
    result.dptr = (char*)((struct storage *)(sto->Object))->srg.data;  
    result.dsize = ((struct storage *)(sto->Object))->srg.size;
    FCLOSE(f);
    return result;
}

/*
CP DataBase::decode_data(datum data) {
    MemoryStream stream(data.dptr, data.dsize);
    File f(&stream);
    bool eof;
    CP d = Parse(&f, eof);  free(data.dptr);  
    return d;
};  // decode_data
*/

at *db_decode_data(data)
datum data;
{
    at *d, *at_sto = new_U8_storage();
    File *f;
    struct storage *sto= at_sto->Object;
    sto->srg.data = sto->allinfo.sts_malloc.addr = data.dptr;
    sto->srg.size = data.dsize;
    sto->srg.flags = STS_MALLOC;
    f = pseudo_fopen(at_sto, "r");
    d = in_at(f);  
    sto->srg.data=sto->allinfo.sts_malloc.addr=NIL;	/* put it back as before */
    sto->srg.size=0;	/* i.e. let whoever malloc'ed data.dptr do the freeing job */
    FCLOSE(f);
    UNLOCK(at_sto);
    return d;
}

/* DATABASE CLASS FUNCTIONS -------------------------- */

void db_dispose(p)
at *p;
{
	DataBase *db=p->Object;
	db_close(db);
	free(db);
	p->Object=NIL;
}

void db_action(p, action)
at *p;
void (* action)();
{
	DataBase *db=p->Object;
	(*action)(db->data_cache);
}

class database_class =
{
  db_dispose,
  db_action,
  generic_name,
  generic_eval,
  generic_listeval,
  XT_DB,
};

/* (db_open <database-filename> <mode>) [mode = "w" or "r"] */
/* returns an XT_DB object */

DX(xdb_open)
{
	DataBase *db = new_db(); /* allocated with malloc */
	char *mode;
	ARG_NUMBER(2);
	ALL_ARGS_EVAL;
	if (arg_array[1]->ctype!=XT_STRING || arg_array[2]->ctype!=XT_STRING)
		error(NIL,"strings expected for the filename and the mode",NIL);
	mode=ASTRING(2);
	db_open(db, ASTRING(1), mode[0]=='w');
	return new_extern(&database_class, db, XT_DB);
}

/* (db_fetch <database> <key>) */
/* returns the lisp object stored under key. */
/* If not found, return NIL */
DX(xdb_fetch)
{
	at *at_db, *key, *ans;
	DataBase *DaBa;

	ARG_NUMBER(2);
	ALL_ARGS_EVAL;
	at_db = APOINTER(1);
	key = APOINTER(2);
	if (at_db && at_db->ctype == XT_DB)
	{	LOCK(at_db);
		DaBa = at_db->Object;
	}
	else error(NIL,"First argument not a database",at_db);
	ans = db_fetch(DaBa, key);
	UNLOCK(at_db);
	return ans;
}

#ifdef KEYBUF
/* (db_next_fetch <database>) */
/* returns the next lisp object stored in the database. */
DX(xdb_next_fetch)
{
	at *at_db, *ans;
	DataBase *DaBa;

	ARG_NUMBER(1);
	ALL_ARGS_EVAL;
	at_db = APOINTER(1);
	if (at_db && at_db->ctype == XT_DB)
	{	LOCK(at_db);
		DaBa = at_db->Object;
	}
	else error(NIL,"First argument not a database",at_db);
	ans = db_next_fetch(DaBa);
	UNLOCK(at_db);
	return ans;
}
#endif

/* (db_create <database-filename>) */
/* returns a new writable database object */
DX(xdb_create)
{
	DataBase *db = new_db(); /* allocated with malloc */
	ARG_NUMBER(1);
	ALL_ARGS_EVAL;
	if (arg_array[1]->ctype!=XT_STRING)
		error(NIL,"A string was expected for the database filename",NIL);
	db_create(db, ASTRING(1));
	return new_extern(&database_class, db, XT_DB);
}

/* (db_store <database> <key> <datum>) */
/* stores the lisp <datum> under <key> */
DX(xdb_store)
{
	at *at_db, *key, *data;
   DataBase *DaBa;

	ARG_NUMBER(3);
	ALL_ARGS_EVAL;
	at_db = APOINTER(1);
	key = APOINTER(2);
	data = APOINTER(3);
	if (at_db && at_db->ctype == XT_DB)
	{	LOCK(at_db);
		DaBa = at_db->Object;
	}
	else error(NIL,"First argument not a database",at_db);
	db_store(DaBa, key, data);
	UNLOCK(at_db);
	LOCK(data);
	return data;

}

/* (db_firstkey <database>) returns the first key in the database */
DX(xdb_firstkey)
{
	at *at_db, *key;
	DataBase *DaBa;

	ARG_NUMBER(1);
	ALL_ARGS_EVAL;
	at_db = APOINTER(1);
	if (at_db && at_db->ctype == XT_DB)
	{	LOCK(at_db);
		DaBa = at_db->Object;
	}
	else error(NIL,"First argument not a database",at_db);
	key = db_firstkey(DaBa);
	UNLOCK(at_db);
	return key;
}

/* (db_nextkey <database> <key>) returns the next key in the database, after <key> */
DX(xdb_nextkey)
{
	at *at_db, *key, *nextk;
	DataBase *DaBa;

	ARG_NUMBER(2);
	ALL_ARGS_EVAL;
	at_db = APOINTER(1);
	key = APOINTER(2);
	if (at_db && at_db->ctype == XT_DB)
	{	LOCK(at_db);
		DaBa = at_db->Object;
	}
	else error(NIL,"First argument not a database",at_db);
	nextk = db_nextkey(DaBa,key);
	UNLOCK(at_db);
	return nextk;
}

/* (db_delete <database> <key>) */
DX(xdb_delete)
{
	at *at_db, *key, *ans;
	DataBase *DaBa;

	ARG_NUMBER(2);
	ALL_ARGS_EVAL;
	at_db = APOINTER(1);
	key = APOINTER(2);
	if (at_db && at_db->ctype == XT_DB)
	{	LOCK(at_db);
		DaBa = at_db->Object;
	}
	else error(NIL,"First argument not a database",at_db);
	db_delete(DaBa, key);
	UNLOCK(at_db);
	/*UNLOCK(key);?*/
	return NIL;
}

/* (db_reorganize <database>) reorganizes the database */
DX(xdb_reorganize)
{
	at *at_db;
	DataBase *DaBa;

	ARG_NUMBER(1);
	ALL_ARGS_EVAL;
	at_db = APOINTER(1);
	if (at_db && at_db->ctype == XT_DB)
	{	LOCK(at_db);
		DaBa = at_db->Object;
	}
	else error(NIL,"First argument not a database",at_db);
	db_reorganize(DaBa);
	UNLOCK(at_db);
	return NIL;
}

/* (dbp <database>) is true if argument is database */
DX(xdbp)
{
	at *at_db;
	DataBase *DaBa;

	ARG_NUMBER(1);
	ALL_ARGS_EVAL;
	at_db = APOINTER(1);
	if (at_db && at_db->ctype == XT_DB)
		return at_db;
	return NIL;
}

/* --------- INITIALISATION CODE --------- */

void
init_gdbm_driver()
{
	class_define("DB", &database_class);

	dx_define("db_create", xdb_create);
	dx_define("db_open", xdb_open);
	dx_define("db_fetch", xdb_fetch);
#ifdef KEYBUF
	dx_define("db_next_fetch", xdb_next_fetch);
#endif
	dx_define("db_store", xdb_store);
	dx_define("db_firstkey", xdb_firstkey);
	dx_define("db_nextkey", xdb_nextkey);
	dx_define("db_delete", xdb_delete);
	dx_define("db_reorganize", xdb_reorganize);
	dx_define("dbp", xdbp);
}

/*
//////////////////////////////////////////////////////////////////////////
// DataBaseCell

DataBaseCell::DataBaseCell(DataBase *d) {
    private_members = public_members;
    db = d ? d : new DataBase();
};  // constructor

DataBaseCell::~DataBaseCell() {
    DeletePrivate(private_members);
    if (db) delete db;
};  // destructor

CellType DataBaseCell::typ() const {
    return DbT;
};  // typ

CP DataBaseCell::EvalFunction(const CP par) {
    CP param = par;
    unsigned index = (unsigned)a_real(param);
    CP key = NewRealCell(index);
    CP data = Nil;
    if (param != Nil) {
        data = a_(param);
        db->Store(key, data);
    } else {
        data = db->Fetch(key);  
    }
    unref(key);
    return data;
};  // EvalFunction

const char* DataBaseCell::GetName() {
    return db->Name();
};  // GetName

Symbol* DataBaseCell::GetSymbol() {
    static Symbol *sym = 0;
    if (!sym) sym = DefineSymbol("DATABASE");
    return sym;
};  // GetSymbol

DataBase* DataBaseCell::GetDataBase() { 
    return db; 
};  // GetDataBase

CLink** DataBaseCell::Public() {
    return &public_members;
};  // Public

CLink** DataBaseCell::Private() {
    return &private_members;
};  // Private


//////////////////////////////////////////////////////////////////////////
// lisp commands

CP ydb_create(CP param) {
    DataBase *db = new DataBase();
    db->Create(a_string(param));
    return new DataBaseCell(db);
};  // ydb_create

CP ydb_open(CP param) {
    DataBase *db = new DataBase();
    String name = a_string(param);
    db->Open(name, check_true(param));
    return new DataBaseCell(db);
};  // ydb_open

CP ydb_close(CP) {
    GetThis()->GetDataBase()->Close();  return Nil;
};  // ydb_close

CP ydb_store(CP param) {
    CP key  = a_(param);
    CP data = a_(param);
    GetThis()->GetDataBase()->Store(key, data);
    unref(key);  return data;
};  // ydb_store

CP ydb_fetch(CP param) {
    CP key = a_(param);
    CP def = param == Nil ? Nil : a_(param);
    CP result = GetThis()->GetDataBase()->Fetch(key, def);
    unref(key);  unref(def);  return result;
};  // ydb_fetch

CP ydb_delete(CP param) {
    CP key = a_(param);
    GetThis()->GetDataBase()->Delete(key);
    unref(key);  return Nil;
};  // ydb_delete

CP ydb_firstkey(CP) {
    return GetThis()->GetDataBase()->FirstKey();
};  // ydb_firstkey

CP ydb_nextkey(CP param) {
    CP key = a_(param);
    CP result = GetThis()->GetDataBase()->NextKey(key);
    unref(key);  return result;
};  // ydb_nextkey

CP ydb_reorganize(CP) {
    GetThis()->GetDataBase()->Reorganize();
    return Nil;
};  // ydb_reorganize

CP ydb_protected(CP) {
    return GetThis()->GetDataBase()->Protected() ? True : Nil;
};  // ydb_protected

CP xdata_basep(CP param) {
    CP t = a_(param);
    bool is_data_base = t->typ() == DbT;  unref(t);
    return is_data_base ? True : Nil;
};  // xdata_basep

static bool db_createS = DefineMember("create", ydb_create, DbT);
static bool db_openS = DefineMember("open", ydb_open, DbT);
static bool db_closeS = DefineMember("close", ydb_close, DbT);
static bool db_storeS = DefineMember("store", ydb_store, DbT);
static bool db_fetchS = DefineMember("fetch", ydb_fetch, DbT);
static bool db_deleteS = DefineMember("delete", ydb_delete, DbT);
static bool db_firstkeyS = DefineMember("firstkey", ydb_firstkey, DbT);
static bool db_nextkeyS = DefineMember("nextkey", ydb_nextkey, DbT);
static bool db_reorganizeS = DefineMember("reorganize", ydb_reorganize, DbT);
static bool db_protectedS = DefineMember("protected", ydb_protected, DbT);
static bool data_basepS = DefineFunction("databasep", xdata_basep);


*/

