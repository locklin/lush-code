#include <math.h>
#include <alloca.h>

#ifndef HCLUSTER_H
#define HCLUSTER_H

/* return the variable in a string 
   (account for the difference between Ansi and K&R cpp) */
#ifdef __STDC__
#define enclose_in_string(a) #a
#else
#define enclose_in_string(a) "a"
#endif

#define CHECK_NULL_POINTER(func, pointer) \
    if(pointer == 0) \
        run_time_error(enclose_in_string(func) \
                       ": bad argument " enclose_in_string(pointer));

#define max(a,b) ((a) >= (b) ? (a) : (b))
#define min(a,b) ((a) <= (b) ? (a) : (b))

typedef struct cluster_type {
    int split;            /* When cluster was splitted */
    void *cluster;        /* pointer to the prototype */
    float variance;       /* average distance between cluster and samples */
    float max_variance;   /* max variance of sub_tree */
    
    int n;		  /* how many sample in this cluster */
    void **sample_index;  /* which samples this cluster is representing */
    float *sample_weight; /* weight for each of the sample */
    
    int *labels;          /* indicates to which cluster each sample is going */
    float weight; 	  /* cluster weight ??? */
    struct cluster_type *child1, *child2; /* sub cluster */
    struct cluster_type *parent; 	  /* parent */

    int label; 		  /* cluster label */
    int spare, spare2, spare3;	  /* to be used by anyway you want */

    /* The following fields are only used by the top node of the tree 
       Yes this is a hack.  There should either be a sublcass for the
       top node, or we should let the user handle it.  But space is cheap... */
    int current_split, current_label;
    
} cluster_type;

extern cluster_type *find_max_variance_node(cluster_type *cluster);

extern double k_means(
    float (*dist) (void *, void *), 	         /* distance */
    void (*clear) (void*), 			 /* clear cluster*/
    void (*accumulate) (void *, float, void *),	 /* accumulate */
    char (*normalize) (void *,  void *, float),  /* normalize cluster*/
    
    int n,			/* number of data points */
    void *(data[]),		/* vector of pointers to samples*/
    float count[],		/* vector of weights for each sample */
    int assign[],		/* contains the labelling on output */
    int p,			/* number of prototypes */
    void *(proto[]),	        /* vector of pointers to prototypes */
    void *(newproto[]),	        /* another for temporary storage */
    int label[],		/* labels associated to each proto */
    float weight[],		/* on output: weight of each proto */
    float variance[],	        /* on output: variance of each proto */
    double var_norm_exp	        /* put the norm factor for var to that power */
    );

extern cluster_type *top_cluster(
    void **sample_index,  /* points on an array of pointers to the samples */
    float *sample_weight, /* points an array of weights to these samples */
    int n,                /* How many samples? */
    void *cluster_center, /* space for cluster center */
    void *temp_cluster,	  /* temp space for cluster */
    void (*init_cluster)(void **sample_index,    /* initialize cluster */
                         float *weight, int n,
                         void **cluster, int p),
    float (*dist) (void *, void *), /* ptr to dist function between samples */
    void (*clear) (void*),/* clear cluster */
    void (*accumulate) (void *, float, void *), /* accumulates samples  */
    char (*normalize) (void *,  void *, float), /* normalize */
    double var_norm_exp	        /* put the norm factor for var to that power */
    );

extern float cluster_split(
    cluster_type *top_cluster,       /* top cluster of the tree */
    cluster_type *cluster,       /* top cluster of the tree */
    void *cluster_center1,	/* space for cluster center1 */
    void *cluster_center2,	/* space for cluster center2 */
    void *temp_cluster1,	/* temp space for cluster1 */
    void *temp_cluster2,	/* temp space for cluster2 */
    void (*init_cluster)(void **sample_index,    /* initialize cluster */
                         float *weight, int n,
                         void **cluster, int p),
    float (*dist) (void *, void *), 	         /* distance */
    void (*clear) (void*), 			 /* clear cluster*/
    void (*accumulate) (void *, float, void *),	 /* accumulate */
    char (*normalize) (void *,  void *, float),   /* normalize cluster*/
    double var_norm_exp	        /* put the norm factor for var to that power */
    );

extern float hcluster_build(
    cluster_type *top_cluster,       /* top cluster of the tree */
    void *cluster_center1,	/* space for cluster center1 */
    void *cluster_center2,	/* space for cluster center2 */
    void *temp_cluster1,	/* temp space for cluster1 */
    void *temp_cluster2,	/* temp space for cluster2 */
    void (*init_cluster)(void **sample_index,    /* initialize cluster */
                         float *weight, int n,
                         void **cluster, int p),
    float (*dist) (void *, void *), 	         /* distance */
    void (*clear) (void*), 			 /* clear cluster*/
    void (*accumulate) (void *, float, void *),	 /* accumulate */
    char (*normalize) (void *,  void *, float),   /* normalize cluster*/
    double var_norm_exp	        /* put the norm factor for var to that power */
    );
    
extern int find_cluster_slice(cluster_type *cluster,  /* top cluster */
    cluster_type **cluster_array, /* space to put the returned cluster */
    int level, int *local_labels, int leaf_number);

extern free_cluster_node(cluster_type *, void (*) (void *));

#endif /* HCLUSTER_H  */

