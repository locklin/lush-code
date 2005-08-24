#ifndef LASVM2_H
#define LASVM2_H

#include "kcache.h"

typedef struct lasvm2_s lasvm2_t;

lasvm2_t *lasvm2_create( lasvm2_kcache_t *cache, int sumflag);

void lasvm2_destroy( lasvm2_t *self );

int lasvm2_get_l( lasvm2_t *self );

int lasvm2_insert( lasvm2_t *self, int xi, int y, double cmin, double cmax );
int lasvm2_process( lasvm2_t *self, int xi, int y, double cmin, double cmax );

int lasvm2_reprocess(lasvm2_t *self, double epsgr);
int lasvm2_finish(lasvm2_t *self, double epsgr);
int lasvm2_solve(lasvm2_t *self, double epsgr);

int lasvm2_get_alpha(lasvm2_t *self, double *alpha);
int lasvm2_get_cmax(lasvm2_t *self, double *cmin);
int lasvm2_get_cmin(lasvm2_t *self, double *cmax);
int lasvm2_get_sv(lasvm2_t *self, int *sv);
int lasvm2_get_g(lasvm2_t *self, double *g);

double lasvm2_get_delta(lasvm2_t *self);
double lasvm2_get_b(lasvm2_t *self);
double lasvm2_get_w2(lasvm2_t *self);

double lasvm2_predict(lasvm2_t *self, int xi);
double lasvm2_predict_nocache(lasvm2_t *self, int xi);

void lasvm2_init( lasvm2_t *self, int l, 
                  const int *sv, const int *y, 
                  const double *alpha, const double *cmin, const double *cmax, 
                  const double *g );


#endif
