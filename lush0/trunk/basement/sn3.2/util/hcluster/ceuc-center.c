#include "ceuc-center.h"

/******************************************************************************
 *
 *  C function for computing centers of cluster.
 *
 *  In particular, C Functions which need to be passed as arguments to the
 *  kmeans algorithms are normaly defined in this file
 *
******************************************************************************/

/******************************************************************************
 *
 *  Allocate and return a new center
 *
******************************************************************************/
struct euc_center_type *euc_new_center()
{
    struct euc_center_type *p;
    p = (struct euc_center_type *)
        malloc(sizeof(struct euc_center_type));
    p->x_size = 0;
    p->y_size = 0;
    p->pat = (float *) malloc(0);
    p->norm = (float *) malloc(0);
    p->norm_back = (float *) malloc(0);
    p->sample_list = 0;
    return p;
}

/******************************************************************************
 *
 *  Free center allocated with euc_new_center
 *
******************************************************************************/
void free_center_list(struct sample_list_type *list);

void euc_free_center(struct euc_center_type *center)
{
    free_center_list(center->sample_list);
    free(center->pat);
    free(center->norm);
    free(center->norm_back);
    free(center);
}
    
/******************************************************************************
 *
 *  Euclidean distance between images of different size
 *
******************************************************************************/
float abs_distance(struct sample_type *sample, struct euc_center_type *center,
                   signed char incx, signed char incy)
{
    float *a = sample->pat, *a_norm = sample->norm, *a_nob = sample->norm_back,
        *b = center->pat, *b_norm = center->norm, *b_nob = center->norm_back,
        dist;

    int ax = sample->x_size, ay = sample->y_size, 
        bx = center->x_size, by = center->y_size;
    int bnx = (ax-bx)/2+incx, bny = (ay-by)/2+incy,
        ax_l = (bnx<0) ? 0 : bnx, ay_l = (bny<0) ? 0 : bny;
    int ax_u = ((bnx+bx > ax) ? ax : bnx+bx) - 1,
        ay_u = ((bny+by > ay) ? ay : bny+by) - 1,
        bx_l = (bnx>0) ? 0 : -bnx, by_l = (bny>0) ? 0 : -bny,
        bx_u = ((ax-bnx > bx) ? bx : ax-bnx) - 1,
        by_u = ((ay-bny > by) ? by : ay-bny) - 1;
    
    /* Integrale of whole image.  If nothing in common, just this */
    dist = *(a_nob + ay*(ax-1)+(ay-1)) +
        *(b_nob + by*(bx-1)+(by-1));
    
    /* debug
      printf("ax %d, ay %d, ainteg %f\nbx %d, by %d, binteg %f\n",
      ax, ay, *(a_nob + ay*(ax-1)+(ay-1)),
      bx, by, *(b_nob + by*(bx-1)+(by-1)));
      */
    
    /* substract integrale over what they have in common */ 
    if((ax_l <= ax_u) && (ay_l <= ay_u)) {
        int i,j,k,l;
        float *a_ptr, *b_ptr;
        
        dist -= (float) integrale_on_square(a_nob, ay, ax_l,
                                            ay_l, ax_u, ay_u);
        dist -= (float) integrale_on_square(b_nob, by, bx_l,
                                            by_l, bx_u, by_u);
        
        /* debug
          printf("ax_l %d, ay_l %d, ax_u %d, ay_u %d, sq integ %f\n",
          ax_l, ay_l, ax_u, ay_u,
          integrale_on_square(a_nob, ay, ax_l,
          ay_l, ax_u, ay_u));
          printf("bx_l %d, by_l %d, bx_u %d, by_u %d, sq integ %f\n",
          bx_l, by_l, bx_u, by_u,
          integrale_on_square(b_nob, by, bx_l,
          by_l, bx_u, by_u));
          */
        
        for(i=ax_l,k=bx_l;i<=ax_u;i++,k++)
            for(j=ay_l,l=by_l, a_ptr = a+ay*i+j, b_ptr = b+by*k+l; 
                j<=ay_u;j++,l++) {
                float diff = (*a_ptr++ - *b_ptr++);
                dist += diff * diff;
            }
        
    }

    return dist;
}
    
float euc_distance(struct sample_type *sample, 
                   struct euc_center_type *center)
{
    int incx, incy;
    float temp, no_move_dist, dist;
    
    no_move_dist = abs_distance(sample, center, 0, 0);
    dist = no_move_dist;
    sample->incx = 0;
    sample->incy = 0;

    for(incx = -1; incx <= 1; incx++)
        for(incy = -1; incy <= 1; incy++) {
            temp = (incx == 0 && incy == 0) ? no_move_dist :
                abs_distance(sample, center, incx, incy);
            /* printf("Cdist = %f\n", temp); */
            if(temp < dist) {
                dist = temp;
                sample->incx = incx;
                sample->incy = incy;
            }
        }
    return dist;
}

/******************************************************************************
 * 
 *  euc_clear: reset center
 *
 ******************************************************************************/
void free_center_list(struct sample_list_type *list)
{
    struct sample_list_type *old;

    while(list) {
        old = list;
        list = list->next;
        free(old);
    }
}

void euc_clear(struct euc_center_type *center)
{
    free_center_list(center->sample_list);
    center->sample_list = 0;
}

/******************************************************************************
 * 
 *  accumulate: add a sample*weight in center (which could be of sample type).
 *
 ******************************************************************************/
void euc_accumulate(struct sample_type *sample,
                    float weight,
                    struct euc_center_type *center)
{
    struct sample_list_type *new_cell = (struct sample_list_type *) 
        malloc(sizeof(struct sample_list_type));
    
    new_cell->next = center->sample_list;
    new_cell->sample = sample;
    new_cell->weight = weight;
    
    center->sample_list = new_cell;
}

/******************************************************************************
 * 
 *  normalize:  take sum_center, and divide it by weight_sum.  Then copy it
 *		into center.  If this operation changes the value of center,
 *		returns 1, othewise returns 0.
 *
 ******************************************************************************/
char euc_normalize(struct euc_center_type *sum_center,
                   struct euc_center_type *center, 
                   float weight_sum)
{
    float x_size=0, x_size_var=0, y_size=0, y_size_var=0, temp;
    struct sample_list_type *p;
    int k;
    char bouge;

    /* Compute the size of the center */

    /* This way use standard deviation
    for(p=sum_center->sample_list;p;p=p->next) {
        temp = p->sample->x_size * p->weight;
        x_size += temp;
        x_size_var += temp * temp;
        temp = p->sample->y_size * p->weight;
        y_size += temp;
        y_size_var += temp * temp;
    }
    if(weight_sum != 0) {
        x_size /= weight_sum;
        x_size_var = sqrt(x_size_var/weight_sum - x_size*x_size);
        y_size /= weight_sum;
        y_size_var = sqrt(y_size_var/weight_sum - y_size*y_size);
    }
    */

    /* This way is max */
    for(p=sum_center->sample_list;p;p=p->next) {
        if(p->sample->x_size > x_size)
            x_size = p->sample->x_size;
        if(p->sample->y_size > y_size)
            y_size = p->sample->y_size;
    }
    
    if(sum_center->x_size != x_size || sum_center->y_size != y_size) {
        sum_center->x_size = x_size;
        sum_center->y_size = y_size;
        sum_center->pat = (float *) realloc(sum_center->pat,
                                            sizeof(float) * x_size*y_size);
        sum_center->norm = (float *) malloc(sum_center->norm,
                                            sizeof(float) * x_size*y_size);
        sum_center->norm_back = (float *) malloc(sum_center->norm_back,
                                                 sizeof(float) * x_size*y_size);
    }

    {
        float *s, *sn, *snb;
        int size = x_size * y_size;
        s = sum_center->pat;
        sn = sum_center->norm;
        snb = sum_center->norm_back;
        for(k=0;k<size;k++) {
            *s++ = 0;
            *sn++ = 0;
            *snb++ = 0;
        }
    }
    /*
    for(p=sum_center->sample_list;p;p=p->next)
        accumulate(p->sample, sum_center);
        */ 

    /*
    norm = 0;
    sum_cl = sum_center->pat;
    cl = center->pat;
    
    bouge = 0;
    for(k=0;k<center->size;k++) {
        *sum_cl /= weight_sum;
        norm += *sum_cl * *sum_cl;
        bouge |= (*sum_cl != *cl);
        *cl++ = *sum_cl++;
    }
    center->norm = norm;
    */
    return bouge;
}

/******************************************************************************
 * 
 *  euc_init_center:  Initialize centers, from a sample_index
 *
 ******************************************************************************/
void euc_init_center(void **sample_index, float *weight, int n, 
                  void **center, int p)
{
    int i,j, size;

    /*
    for(i=0;i<p;i++) {
        struct sample_type *s_ptr = (struct sample_type *) sample_index[i],
            *c_ptr = (struct sample_type *) center[i];

        for(j=0;j<s_ptr->size;j++)
            c_ptr->pattern[j] = s_ptr->pattern[j];
        c_ptr->norm = s_ptr->norm;
        
    }
    */
}
