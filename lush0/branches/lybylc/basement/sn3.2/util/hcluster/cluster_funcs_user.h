#ifndef CLUSTER_FUNCS_USER_H
#define CLUSTER_FUNCS_USER_H

/******************************************************************************
 *
 *	C structre to represent samples and cluster center.  Note that 
 * 	these could have two different C structures.  In this case, the same
 *	structure is used (a lot more info is kept about the cluster center
 *	in the structure cluster_type, which is defined and administered in
 *	hcluster.h and hcluster.c.
 *
******************************************************************************/
struct sample_type {
    float *pattern;	/* data vector */
    float norm;		/* norm of the vector (to compute dot product faster)*/
    int size;		/* size of the vector */
};

extern float distance(struct sample_type *sample, struct sample_type *cluster);
extern void clear(struct sample_type *sample);
extern void accumulate(struct sample_type *sample,
                       float weight,
                       struct sample_type *cluster);
extern char normalize(struct sample_type *sum_cluster,
                      struct sample_type *cluster, 
                      float weight_sum);
extern void cluster_init(void **sample_index, float *weight, int n,
                         void **cluster, int p);

extern void *new_sample(int size);
extern void free_sample(void *v_ptr);

#endif /* CLUSTER_FUNCS_USER_H */ 
