#ifndef LASVM_H
#define LASVM_H

#include "kcache.h"

typedef struct lasvm_s lasvm_t;

lasvm_t *lasvm_create( lasvm_kcache_t *cache,
                       int sumflag, double cp, double cn );

void lasvm_destroy( lasvm_t *self );

int lasvm_get_l( lasvm_t *self );

int lasvm_process( lasvm_t *self, int xi, double y );

int lasvm_reprocess(lasvm_t *self, double epsgr);

int lasvm_finish(lasvm_t *self, double epsgr);

double lasvm_get_cp( lasvm_t *self );

double lasvm_get_cn( lasvm_t *self );

double lasvm_get_delta(lasvm_t *self);

int lasvm_get_alpha(lasvm_t *self, double *alpha);

int lasvm_get_sv(lasvm_t *self, int *sv);

int lasvm_get_g(lasvm_t *self, double *g);

double lasvm_get_b(lasvm_t *self);

double lasvm_get_w2(lasvm_t *self);

double lasvm_predict(lasvm_t *self, int xi);

double lasvm_predict_nocache(lasvm_t *self, int xi);

void lasvm_init( lasvm_t *self, int l, 
                 const int *sv, 
                 const double *alpha, 
                 const double *g );

#endif
