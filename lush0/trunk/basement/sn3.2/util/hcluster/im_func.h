#ifndef IM_FUNC_H
#define IM_FUNC_H

/******************************************************************************
 *
 *	Structure for sample
 *
******************************************************************************/
struct sample_type {
    /* image size (y_size varies quicker than x_size) */
    int x_size, y_size;	
    int hot_x, hot_y;

    /* image at full resolution */
    float *pat, *norm, *norm_back;
    float background;

    signed char incx, incy;

    int spare;
};


/******************************************************************************
 *
 *	Structure for cluster_center
 *
******************************************************************************/
/* The following structure is to hold a list of all the samples associated 
 * with the center.  This allow the procdure normalize to make multiple pass
 * on the samples.
 */
struct sample_list_type {
    struct sample_type *sample;
    struct sample_list_type *next;
    float weight;
};

struct euc_center_type {
    /* image size (y_size varies quicker than x_size) */
    int x_size, y_size;	

    /* image at full resolution */
    float *pat, *norm, *norm_back;

    struct sample_list_type *sample_list;
};

/******************************************************************************
 *
 *	External functions
 *
******************************************************************************/
extern void update_integral(int x_size, int y_size,
                            float *pat, float *norm, float *norm_back, 
                            float background);

extern float integrale_on_square(float *mat, int y_size,
                                 int ux, int uy, int lx, int ly);


extern void sample_set_spare(struct sample_type *sample, int spare);
extern int sample_get_spare(struct sample_type *sample);

extern void set_euc_distance_range(int incx, int incy);
extern int get_euc_distance_incx() ;
extern int get_euc_distance_incy() ;

extern struct euc_center_type *euc_new_center(int size);
extern void euc_free_center(struct euc_center_type *center);

extern float euc_distance(struct sample_type *sample,
                          struct euc_center_type *center);

extern void euc_clear(struct euc_center_type *sample);

extern void euc_accumulate(struct sample_type *sample,
                           float weight,
                           struct euc_center_type *center);

extern void set_euc_normalize_recompute(int flag);
extern int get_euc_normalize_recompute();
extern char euc_normalize(struct euc_center_type *sum_center,
                          struct euc_center_type *center, 
                          float weight_sum);

extern void euc_init_center(void **sample_index, float *weight, int n,
                            void **center, int p);

#endif /* IM_FUNC_H */

