#include "euc_func.h"

/******************************************************************************
 *
 *  C Functions which need to be passed as arguments to the kmeans and the
 *  hiearchical clustering algorithms.
 *
******************************************************************************/

/******************************************************************************
 *
 *  distance: compute the euclidean distance between two sample_type 
 *  structures.
 *
******************************************************************************/
float distance(struct sample_type *sample, struct sample_type *cluster)
{
    int k;
    float *sa, *cl, di;
    
    sa = sample->pattern;
    cl = cluster->pattern;
    di = 0;
    for(k=0;k<sample->size;k++) {
        di += *sa++ * *cl++;
    }
    di = sample->norm + cluster->norm - 2*di; 
    
    return di;
}

/******************************************************************************
 * 
 *  new_sample: allocate and return a pointer to a sample.  
 *  
 *	It returns a void* because this function will be passed as an
 *      argument to functions which knows nothing about 'struct sample_type'.
 *
 ******************************************************************************/
void *new_sample(int size)
{
    struct sample_type *c_ptr;
    
    c_ptr = (struct sample_type *) malloc(sizeof(struct sample_type));
    if(!c_ptr)
        run_time_error("new_samples: not enough memory");
    c_ptr->pattern = (float *) malloc(sizeof(float) * size);
    if(!c_ptr->pattern)
        run_time_error("new_samples: not enough memory");
    c_ptr->size = size;

    return (void *) c_ptr;
}

/******************************************************************************
 *
 * free_sample: free memory allocated by new_sample
 *
 ******************************************************************************/
void free_sample(void *v_ptr)
{
    struct sample_type *c_ptr = (struct sample_type *) v_ptr;

    free(c_ptr->pattern);
    free(c_ptr);
}

/******************************************************************************
 * 
 *  clear: set a pattern to 0
 *
 ******************************************************************************/
void clear(struct sample_type *sample)
{
    float *cl;
    int k;
    
    cl = sample->pattern;
    for(k=0;k<sample->size;k++) 
        *cl++ = 0;
    sample->norm = 0;
}

/******************************************************************************
 * 
 *  accumulate: add a sample*weight in cluster (which could be of sample type).
 *
 ******************************************************************************/
void accumulate(struct sample_type *sample,
		float weight,
		struct sample_type *cluster)
{
    float *sa, *cl;
    int k;
    
    sa = sample->pattern;
    cl = cluster->pattern;
    for(k=0;k<sample->size;k++)
        *cl++ += *sa++ * weight;
}

/******************************************************************************
 * 
 *  normalize:  take sum_cluster, and divide it by weight_sum.  Then copy it
 *		into cluster.  If this operation changes the value of cluster,
 *		returns 1, othewise returns 0.
 *
 ******************************************************************************/
char normalize(struct sample_type *sum_cluster,
	       struct sample_type *cluster, 
	       float weight_sum)
{
    float *sum_cl, *cl, norm;
    int k;
    char bouge;
    
    norm = 0;
    sum_cl = sum_cluster->pattern;
    cl = cluster->pattern;
    
    bouge = 0;
    for(k=0;k<cluster->size;k++) {
        float temp = *sum_cl / weight_sum;
        norm += temp * temp;
        bouge |= (temp != *cl);
        *cl++ = temp;
        sum_cl++;
    }
    cluster->norm = norm;
    return bouge;
}

/******************************************************************************
 * 
 *  cluster_init:  Initialize clusters, from a sample_index
 *
 ******************************************************************************/
void cluster_init(void **sample_index, float *weight, int n, 
                  void **cluster, int p)
{
    int i,j, size, min_n_p = (n<p)?n:p;

    /* copy first min(n,p) samples into clusters */
    for(i=0;i<min_n_p;i++) {
        struct sample_type *s_ptr = (struct sample_type *) sample_index[i],
            *c_ptr = (struct sample_type *) cluster[i];

        for(j=0;j<c_ptr->size;j++)
            c_ptr->pattern[j] = s_ptr->pattern[j];
        c_ptr->norm = s_ptr->norm;
    }

    /* if any cluster left, initialize to zero */
    for(i=min_n_p;i<p;i++) {
        struct sample_type *c_ptr = (struct sample_type *) cluster[i];
        for(j=0;j<c_ptr->size;j++)
            c_ptr->pattern[j] = 0;
        c_ptr->norm = 0;
    }
    
}
		



