#include "im_func.h"

/******************************************************************************
 *
 *  C Functions used to build and manage samples, and for computing cluster
 *  center.
 *
******************************************************************************/

/******************************************************************************
 *
 *  Compute integrals:
 *
 *  p->norm[p->y_size * x + y] =
 *  	sum_{i=0,j=0}^{i=x,j=y} p->pat[p->y_size*i+j]**2.  
 *  p->norm_back[p->y_size * x + y] =
 *  	sum_{i=0,j=0}^{i=x,j=y} (p->pat[p->y_size*i+j]-background)**2.  
 *
******************************************************************************/
void update_integral(int x_size, int y_size,
                            float *pat, float *norm, float *norm_back, 
                            float background)
{
    float *norm_line, *norm_b_line;
    int i,j;

    norm_line = (float *) alloca(y_size*sizeof(float));
    norm_b_line = (float *) alloca(y_size*sizeof(float));
    
    for(j=0;j<y_size;j++) {
        norm_line[j] = 0;
        norm_b_line[j] = 0;
    }
    
    for(i=0;i<x_size;i++) {
        float sum_norm = 0, sum_norm_b = 0;
        for(j=0;j<y_size;j++) {
            float diff = pat[y_size*i+j];
            sum_norm += diff * diff;
            diff -= background;
            sum_norm_b += diff * diff;
            norm_line[j] += sum_norm;
            norm_b_line[j] += sum_norm_b;
            norm[y_size*i+j] = norm_line[j];
            norm_back[y_size*i+j] = norm_b_line[j];
        }
    }
}

/******************************************************************************
 *
 *  Compute an integral on a rectagle quickly using a table (computed above)
 *
 *  Returns: 
 *  	sum_{i=lx,j=ly}^{i=ux,j=uy} mat[y_size*i+j]
 *
******************************************************************************/
float integrale_on_square(float *mat, int y_size,
                          int ux, int uy, int lx, int ly)
{
    /*
    printf("ux = %d, uy=%d loc=%d, add %x, %f\n", ux, uy, y_size*lx+ly, 
           mat, *(mat+y_size*lx+ly));
           */

    if(ux==0)
        if(uy == 0) 
            return *(mat+y_size*lx+ly);
        else
            return *(mat+y_size*lx+ly) - *(mat+y_size*lx+(uy-1));
    else
        if(uy == 0)
            return *(mat+y_size*lx+ly) - *(mat+y_size*(ux-1)+ly);
        else
            return (*(mat+y_size*lx+ly) - *(mat+y_size*(ux-1)+ly))
            - (*(mat+y_size*lx+(uy-1)) - *(mat+y_size*(ux-1)+(uy-1)));
}

/******************************************************************************
 *
 *  set and get spare field
 *
******************************************************************************/
void sample_set_spare(struct sample_type *sample, int spare)
{
    if(sample)
        sample->spare = spare;
    else
        run_time_error("sample_set_spare: null pointer");
}

int sample_get_spare(struct sample_type *sample)
{
    if(sample)
        return sample->spare;
    else
        run_time_error("sample_get_spare: null pointer");
}

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
struct euc_center_type *euc_new_center(int size)
{
    struct euc_center_type *p;
    p = (struct euc_center_type *)
        malloc(sizeof(struct euc_center_type));
    if(!p) run_time_error("euc_new_center: not enough memory");
    p->x_size = 1;
    p->y_size = size;
    p->pat = (float *) malloc(sizeof(float) * size);
    p->norm = (float *) malloc(sizeof(float) * size);
    p->norm_back = (float *) malloc(sizeof(float) * size);

    if(!p->pat || !p->norm || !p->norm_back)
        run_time_error("euc_new_center: not enough memory");
    p->sample_list = 0;
    return p;
}

void print_debut(float *truc)
{
    int i;
    for(i=0;i<4;i++)
        printf(" %8.2f", truc[i]);
    printf("\n");
}

/******************************************************************************
 *
 *  Free center allocated with euc_new_center
 *
******************************************************************************/
void free_center_list(struct sample_list_type *list);

void euc_free_center(struct euc_center_type *center)
{
    if(center == 0)
        run_time_error("euc_free_center: Null pointer");
        
    free_center_list(center->sample_list);
    free(center->pat);
    free(center->norm);
    free(center->norm_back);
    free(center);
}
    
/******************************************************************************
 *
 *  Euclidean distance between images of different sizes
 *
 *            ------------      
 *            |          |            Image A, is composed of A1, A2, A3
 *            |    A1    |            Image B, is composed of B1, B2, B3
 *            |          |      
 *      ------------------------      Image A and B after centering (using 
 *      |     |          |     |      either hot spots or centers) overlap
 *      | B1  |  A2  B2  | B3  |      on A2 and B2
 *      |     |          |     |
 *      ------------------------      
 *            |          |      
 *            |    A3    |       
 *            |          |            
 *            ------------            
 *
 *  The distance is the following (back stands for background):
 *
 *  Dist(A, B) =  sum_{b in B} (b - back)^2           
 *		+ sum_{a in A} (a - back)^2           
 *		- sum_{a in A2} (a-back)^2
 *		- sum_{b in B2} (b-back)^2
 *		+ sum_{a in A2, b in B2} (a-b)^2 
 *
 *  where B = B1 U B2 U B3, A = A1 U A2 U A3, and the last sum is done by
 *  making a and b correspond in their respective position in A2 and B2.
 *
 *  Assuming A2 and B2 are the overlap between the two images, this formula
 *  works for any kind of overlap, assuming that the background are the same
 *  for both images.
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
    /* center using center of each images 
       int bnx = (ax-bx)/2 + incx, bny = (ay-by)/2 + incy; */
    /* center using hot point */
    int bnx = sample->hot_x - bx/2 + incx, 
        bny = sample->hot_y - by/2 + incy; 
    int ax_l = (bnx<0) ? 0 : bnx, ay_l = (bny<0) ? 0 : bny;
    int ax_u = ((bnx+bx > ax) ? ax : bnx+bx) - 1,
        ay_u = ((bny+by > ay) ? ay : bny+by) - 1,
        bx_l = (bnx>0) ? 0 : -bnx, by_l = (bny>0) ? 0 : -bny,
        bx_u = ((ax-bnx > bx) ? bx : ax-bnx) - 1,
        by_u = ((ay-bny > by) ? by : ay-bny) - 1;

    /*
printf("SIZE: %d %d %d %d\n", ax, ay, bx, by);
print_debut(sample->pat);
print_debut(center->pat);
print_debut(sample->norm);
print_debut(center->norm);
print_debut(sample->norm_back);
print_debut(center->norm_back);
*/
    
    /* Integrale of whole image.  If nothing in common, just this */
    dist = *(a_nob + ay*(ax-1)+(ay-1)) +
        *(b_nob + by*(bx-1)+(by-1));
    /*
printf("******** DIST1 = %f\n", *(a_nob + ay*(ax-1)+(ay-1)));
printf("******** DIST2 = %f\n", *(b_nob + by*(bx-1)+(by-1)));
*/
    
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

        /*
        for(i=0;i<ax*ay;i++)
            if(a_nob[i] != b_nob[i])
                printf("DIFF %d %f %f\n", i, a_nob[i], b_nob[i]);
        
        printf("BOUNDARY ax=%d ay=%d, ax_l=%d, ay_l=%d\n", ax, ay, ax_l, ay_l);

        printf("loc = %d, add = %x, integ = %f\n", ay*ax_u+ay_u, a_nob, 
               *(a_nob+ay*ax_u+ay_u));
        printf("loc = %d, add = %x, integ = %f\n", by*bx_u+by_u, b_nob,
               *(b_nob+by*bx_u+by_u));
        
printf("%d %d %d %d %d\n", ay, ax_l, ay_l, ax_u, ay_u);
printf("%d %d %d %d %d\n", by, bx_l, by_l, bx_u, by_u);

printf("******** DIST1 = %f\n", (float) integrale_on_square(a_nob, ay, ax_l,
                                            ay_l, ax_u, ay_u));
printf("******** DIST2 = %f\n", (float) integrale_on_square(b_nob, by, bx_l,
                                            by_l, bx_u, by_u));
                                            */
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
            for(j=ay_l, a_ptr = a+ay*i+ay_l, b_ptr = b+by*k+by_l; 
                j<=ay_u;j++) {
                float diff = (*a_ptr++ - *b_ptr++);
                dist += diff * diff;
            }
        
    }

    return dist;
}

/******************************************************************************
 *
 *  void set_euc_distance_range(int incx, int incy)
 *
 *  calling set_euc_distance_range has a side effect on how euc_distance 
 *  compute.  euc_distance position sample on all the positions in the 
 *  rectangle (-incx,-incy) (incx, incy), to find the minimum euclidean 
 *  distance.
 *
******************************************************************************/
static int euc_distance_incx = 0;
static int euc_distance_incy = 0;

void set_euc_distance_range(int incx, int incy)
{
    euc_distance_incx = incx;
    euc_distance_incy = incy;
}
     
int get_euc_distance_incx() 
{ 
    return euc_distance_incx;
}

int get_euc_distance_incy() 
{ 
    return euc_distance_incy;
}

/******************************************************************************
 *
 *  Compute the euc_distance between sample and center (not necessarily of 
 *  same size).  Iterate to find the best position to place sample.
 *
******************************************************************************/
float euc_distance(struct sample_type *sample, 
                   struct euc_center_type *center)
{
    int incx, incy;
    float temp, no_move_dist, dist;
    
    no_move_dist = abs_distance(sample, center, 0, 0);
    dist = no_move_dist;
    sample->incx = 0;
    sample->incy = 0;

    for(incx = -euc_distance_incx; incx <= euc_distance_incx; incx++)
        for(incy = -euc_distance_incy; incy <= euc_distance_incy; incy++) {
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


float abs_accumulate(struct sample_type *sample, float weight,
                     struct euc_center_type *center,
                     signed char incx, signed char incy)
{
    float *a = sample->pat, *a_norm = sample->norm, *a_nob = sample->norm_back,
        *b = center->pat, *b_norm = center->norm, *b_nob = center->norm_back,
        dist;

    int ax = sample->x_size, ay = sample->y_size, 
        bx = center->x_size, by = center->y_size;
    /* center using center of each images 
       int bnx = (ax-bx)/2 + incx, bny = (ay-by)/2 + incy; */
    /* center using hot point */
    int bnx = sample->hot_x - bx/2 + incx, 
        bny = sample->hot_y - by/2 + incy; 
    int ax_l = (bnx<0) ? 0 : bnx, ay_l = (bny<0) ? 0 : bny;
    int ax_u = ((bnx+bx > ax) ? ax : bnx+bx) - 1,
        ay_u = ((bny+by > ay) ? ay : bny+by) - 1,
        bx_l = (bnx>0) ? 0 : -bnx, by_l = (bny>0) ? 0 : -bny,
        bx_u = ((ax-bnx > bx) ? bx : ax-bnx) - 1,
        by_u = ((ay-bny > by) ? by : ay-bny) - 1;
    
    /* debug
      printf("ax %d, ay %d, ainteg %f\nbx %d, by %d, binteg %f\n",
      ax, ay, *(a_nob + ay*(ax-1)+(ay-1)),
      bx, by, *(b_nob + by*(bx-1)+(by-1)));
      */
    
    /* substract integrale over what they have in common */ 
    if((ax_l <= ax_u) && (ay_l <= ay_u)) {
        int i,j,k,l;
        float *a_ptr, *b_ptr, background = sample->background;
        
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
            for(j=ay_l, a_ptr = a+ay*i+ay_l, b_ptr = b+by*k+by_l; 
                j<=ay_u;j++)
                *b_ptr++ += (*a_ptr++ - background)*weight;
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

    if(!new_cell)
        run_time_error("euc_accumulate: not enough memory");
    new_cell->next = center->sample_list;
    new_cell->sample = sample;
    new_cell->weight = weight;
    
    center->sample_list = new_cell;
}

/******************************************************************************
 * 
 *  void set_euc_normalize_recompute(int flag)
 *
 *  Calling this function has a side effect on euc_normalize.  Normally, 
 *  this function does not recompute the center of the cluster according to
 *  the best possible positions of the clusters.  However a call to this 
 *  function with flag=1 will force just that.
 *
******************************************************************************/
static int euc_normalize_recompute = 0;

void set_euc_normalize_recompute(int flag)
{
    euc_normalize_recompute = flag;
}

int get_euc_normalize_recompute()
{
    return euc_normalize_recompute;
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

    /* This way is max.  This is trickier than it seems because we
       want to insure that the distance between a sample and itself as a
       cluster center is zero.  The hot spot for the cluster center is the
       middle of the image, so we must make sure that when the sample is
       added, the cluster center is large enough to add all of the sample's
       pixels */
    for(p=sum_center->sample_list;p;p=p->next) {
        if(2*p->sample->hot_x > p->sample->x_size)
            temp = 2*p->sample->hot_x;
        else
            temp = 2*(p->sample->x_size - p->sample->hot_x);
        if(temp > x_size)
            x_size = temp;
        if(2*p->sample->hot_y > p->sample->y_size)
            temp = 2*p->sample->hot_y;
        else
            temp = 2*(p->sample->y_size - p->sample->hot_y);
        if(temp > y_size)
            y_size = temp;
        /*
        printf("Sample: %d,%d hot: %d,%d  Max: %f,%f\n",
               p->sample->x_size, p->sample->y_size, p->sample->hot_x, p->sample->hot_y,
               x_size, y_size);
               */
    }
    
    /* printf("x_size = %d, y_size = %d\n",
       (int) (x_size+0.5), (int) (y_size+0.5)); */

    sum_center->x_size = x_size;
    sum_center->y_size = y_size;
    sum_center->pat = (float *) 
        realloc(sum_center->pat,
                (int) (sizeof(float) * x_size*y_size+.5));
    sum_center->norm = (float *) 
        realloc(sum_center->norm,
                (int) (sizeof(float) * x_size*y_size + 0.5));
    sum_center->norm_back = (float *) 
        realloc(sum_center->norm_back,
                (int) (sizeof(float) * x_size*y_size+0.5));
    if(!sum_center->pat || !sum_center->norm || !sum_center->norm_back)
        run_time_error("euc_normalize: not enough memory");

    {
        float *s, *c, background = 
            sum_center->sample_list->sample->background*weight_sum;
        int size = x_size * y_size;

        s = sum_center->pat;
        for(k=0;k<size;k++)
            *s++ = background;

        for(p=sum_center->sample_list;p;p=p->next)
            abs_accumulate(p->sample, p->weight, sum_center, 0, 0);

        s = sum_center->pat;
        for(k=0;k<size;k++)
            *s++ /= weight_sum;

        if(euc_normalize_recompute) {
            update_integral(sum_center->x_size, sum_center->y_size, 
                            sum_center->pat, sum_center->norm,
                            sum_center->norm_back, background);
            
            for(p=sum_center->sample_list;p;p=p->next)
                euc_distance(p->sample, sum_center);
            
            s = sum_center->pat;
            for(k=0;k<size;k++)
                *s++ = background;
            
            for(p=sum_center->sample_list;p;p=p->next)
                abs_accumulate(p->sample, p->weight, sum_center, 
                               p->sample->incx, p->sample->incy);
            
            s = sum_center->pat;
            for(k=0;k<size;k++)
                *s++ /= weight_sum;
        }

        bouge = 0;

        if(center->x_size != x_size || center->y_size != y_size) {
            bouge = 1;

            center->x_size = x_size;
            center->y_size = y_size;
            center->pat = (float *) 
                realloc(center->pat,
                        (int) (sizeof(float) * x_size*y_size+.5));
            center->norm = (float *) 
                realloc(center->norm,
                        (int) (sizeof(float) * x_size*y_size + 0.5));
            center->norm_back = (float *) 
                realloc(center->norm_back,
                        (int) (sizeof(float) * x_size*y_size+0.5));
            if(!center->pat || !center->norm || !center->norm_back)
                run_time_error("euc_normalize: not enough memory");

            s = sum_center->pat;
            c = center->pat;
            for(k=0;k<size;k++) {
                *c++ = *s++;
            }
        } else {
            s = sum_center->pat;
            c = center->pat;
            for(k=0;k<size;k++) {
                bouge |= (*s != *c);
                *c++ = *s++;
            }
        }

        update_integral(center->x_size, center->y_size, 
                        center->pat, center->norm, center->norm_back, 
                        background);
    }
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
    struct sample_type **samples = (struct sample_type **) sample_index;
    struct euc_center_type **centers = (struct euc_center_type **) center;
    int i;

    if(p>n)
        run_time_error("euc_init_center: more clusters than samples");
    
    if(p == 2) {
        /* special: take the smallest sized pattern */
        int size, min_size, max_size, small=0, big=0;

        min_size = samples[0]->x_size * samples[0]->y_size;
        max_size = min_size;

        for(i=1;i<n;i++) {
            size = samples[i]->x_size * samples[i]->y_size;

            if(min_size > size) {
                min_size = size;
                small = i;
            }
            if(max_size < size) {
                max_size = size;
                big = i;
            }
        }

        euc_clear(centers[0]);
        euc_accumulate(samples[small], 1.0, centers[0]);
        euc_normalize(centers[0], centers[0], 1);

        euc_clear(centers[1]);
        euc_accumulate(samples[big], 1.0, centers[1]);
        euc_normalize(centers[1], centers[1], 1);
        
    } else {

        for(i=0;i<p;i++) {
            float *s, *sn, *snb, *c, *cn, *cnb;
            int j, size;
            
            struct sample_type *s_ptr = (struct sample_type *) sample_index[i];
            struct euc_center_type *c_ptr = (struct euc_center_type *) center[i];
            
            euc_clear(c_ptr);
            euc_accumulate(s_ptr, 1.0, c_ptr);
            euc_normalize(c_ptr, c_ptr, 1);
        }
    }
    
}
