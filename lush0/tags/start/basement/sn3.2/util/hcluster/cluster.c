#include "cluster.h"

void find_closest(void *(data[]), int n, void *(proto[]), int p, int *closest,
                  float *distance, float (*dist) (void *, void *), char *bouge);
float recompute_proto(int n, void **data, float *count, int *closest, int p,
                      void **proto, void **newproto, float *weight, char *bouge,
                      float *distance, float *variance, 
                      float (*dist) (void *, void *), void (*clear) (void*),
                      void (*accumulate) (void *, float, void *),
                      char (*normalize) (void *,  void *, float));
float on_line_first(int n, void **data, float *count, int *closest, int p,
                    void **proto, void **newproto, float *weight, char *bouge,
                    float *distance, float *variance, 
                    float (*dist) (void *, void *), void (*clear) (void*),
                    void (*accumulate) (void *, float, void *),
                    char (*normalize) (void *,  void *, float));

/******************************************************************************
 * Clustering.
 * 
 * The clusering functions are k-means (for clustering n sample in p clusters),
 * and top-cluster and build-cluster (for hierarchical clustering).  top-cluster
 * is an initalization routine while build-cluster is used to incrementally 
 * cluster the data into a tree structure.  Since these functions work on
 * any kind of data structure, the user is expected to provide pointers to 
 * functions to manipuate the data.  We assume that the data is made of C
 * structures (let's calling sample_type) defined by the user, and that these C
 * structures can be used to represent both the sample and the cluster center.
 * Note that it is possible to have two different data structure for samples
 * and cluster center, and in that case, some care must be taken in the
 * in passing functions pointers (for instance, when dist is called by k_means,
 * the first argument is a sample, the second is a cluster center).
 * The user is expected to provide pointers to the following
 * functions to manipulate this data (we assume that <sa>, <clu> and 
 * <newclu> are pointers to sample_type):
 *
 *   dist(<sa>, <clu>) 
 *       compute distance between sample <sa> and cluster <clu>.  Since this
 *	 function will be called many time by the clustering algorithm, it 
 *       should be reasonably efficient.
 *
 *   The following three functions ("clear", "accumulate" and "normalize") 
 *   work together and are used to compute the position of one cluster.
 *   "clear" reset a cluster, "accumulate" iteratively add information about
 *   each sample to that cluster, and "normalize" is called last to compute
 *   the true position of the cluster (using all the information accumulated 
 *   with "accumulate").  For example if the euclidean distance is used and 
 *   each sample is a N dimensional vector, "clear" could set the cluster to 
 *   the zero vector, "accumulate" could add each samples to the cluster, and
 *   "normalize" could divide each component of the cluster by the number of
 *   samples.
 *
 *   clear(<clu>)
 *       Clear a sample or a cluster center.
 *   accumulate(<sa>, <weight>, <clu>)
 *       Accumulate information about sample pointed to by <sa> and weighted
 *       by <weight> (a float) into cluster <clu>.
 *   normalize(<clu1>, <newclu>, <weight_sum>)
 *       <clu1> is the unfinished cluster which has been passed to accumulate.
 *       <newclu> is the normalized pattern.  <weight_sum> should be the sum 
 *       of all the <weight>s passed to accumulate.  It's a float.  On entry,
 *       <newclu> has the previous value for the cluster.  If <newclu>
 *       is modified by the normalize call, the procedure should return 1,
 *       otherwise 0.
 *
 *   The following function is used to initalize the cluster centers as
 *   a function of the samples.  The simplest way to do this is to copy
 *   the first samples into the cluster centers.
 *
 *   init_cluster(&<sa>, <sa_size>, &<clu>, <clu_size>)
 *       Initialize the cluster as a function of the samples.  The first 
 *       argument is a pointer to an array of pointers to sample_type 
 *       structures.  The size of the array is given by <sa_size> (an int).
 *       The third argument is a pointer to an array of pointer to sample_type,
 *       representing the centers of clusters.  The array size is given in
 *       <clu_size> (an int).
 *
******************************************************************************/

/******************************************************************************
 * kmeans clustering
 * 
 * dist: a distance function between prototype and sample
 * accumulate: an accumulation function that takes a sample and updates
 *             the new prototype with it (this may actually just add
 *             the sample to a list attached to the proto if the cluster
 *             center calculation is non incremental
 * normalize: construct cluster center from "accumulated" samples
 * n: number of samples
 * data: vector of pointer to samples
 * count: vector of weight for each sample
 * assign: vector of labels for each sample (filled up by the function)
 * p: number of prototypes
 * proto: vector of pointers to prototypes
 * newproto: vector of pointers to prototype/cluster accumulators
 * label: vector of labels associated with cluster
 * weight: vector of weights for each cluster (filled up by the function)
 * variance: vector of variances for each cluster (filled up by the function)
 * var_norm_pow: double, put the norm factor for var to that power
******************************************************************************/
double k_means(
	/* ptr to distance function between vectors */
	float (*dist) (void *, void *), 
        
	/* accumulates samples in the proto */
	void (*clear) (void*), 

	/* accumulates samples in the proto */
	void (*accumulate) (void *, float, void *),

	/* normalization */
	char (*normalize) (void *,  void *, float),

	int n,			/* number of data points */

	void *(data[]),		/* vector of pointers to samples*/

 	float count[],		/* vector of weights for each sample */
	int assign[],		/* contains the labelling on output */
	int p,			/* number of prototypes */
	void *(proto[]),	/* vector of pointers to prototypes */
	void *(newproto[]),	/* another for temporary storage */
	int label[],		/* labels associated to each proto */
	float weight[],		/* on output: weight of each proto */
	float variance[], 	/* on output: variance of each proto */
        double var_norm_exp	/* put the norm factor for var to that power */
        )
{
    char *bouge;
    int k, i, it = 0, *closest, *old_closest;
    double old_variance = MAXFLOAT, sum_variance;
    float *distance;
  
    CHECK_NULL_POINTER(k_means,dist);
    CHECK_NULL_POINTER(k_means,clear);
    CHECK_NULL_POINTER(k_means,accumulate);
    CHECK_NULL_POINTER(k_means,normalize);
    CHECK_NULL_POINTER(k_means,data);
    CHECK_NULL_POINTER(k_means,count);
    CHECK_NULL_POINTER(k_means,assign);
    CHECK_NULL_POINTER(k_means,proto);
    CHECK_NULL_POINTER(k_means,newproto);
    CHECK_NULL_POINTER(k_means,label);
    CHECK_NULL_POINTER(k_means,weight);
    CHECK_NULL_POINTER(k_means,variance);

    /* initialize initial assignements of cluster to sample */
    closest = (int *) malloc(sizeof(int)*n);
    old_closest = (int *) malloc(sizeof(int)*n);
    distance = (float *) malloc(sizeof(float)*n);
    bouge = (char *) malloc(sizeof(char)*p);
    if(!closest || !old_closest || !distance || !bouge) 
        run_time_error("Out of memory in k_means");

    /* Assign each sample to a prototype */
    for (k=0; k<n; k++)
        closest[k] = 0;
    for (i=0; i<min(n,p); i++) {
        closest[i] = i;
        bouge[i] = 1;
    }
    for (k=0; k<n; k++)
        distance[k] = dist(data[k], proto[closest[k]]);

    sum_variance = recompute_proto(n, data, count, closest, p, proto, 
                                   newproto, weight, bouge, distance, 
                                   variance, dist, clear, accumulate,
                                   normalize);

/* 
    sum_variance = on_line_first(n, data, count, closest, p, proto, 
                                   newproto, weight, bouge, distance, 
                                   variance, dist, clear, accumulate,
                                   normalize);
*/

    /* for as long as progress is made*/
    while (old_variance > sum_variance) {
        
        /* for each sample, find closest prototype */
        for (k=0; k<n; k++)
            old_closest[k] = closest[k];
        find_closest(data, n, proto, p, closest, distance, dist, bouge);

        /* recompute prototypes which have moved, and variance */
        old_variance = sum_variance;
        sum_variance = recompute_proto(n, data, count, closest, p, proto, 
                                       newproto, weight, bouge, distance, 
                                       variance, dist, clear, accumulate,
                                       normalize);
        it++;

    } 

/*
 if(n*it>1000)
    printf("n = %5d, it = %4d  product = %d\n", n, it, n*it);
*/
    /* If energy has increased, go back to lower energy */
    if(sum_variance > old_variance) {
        for (k=0; k<n; k++)
            closest[k] = old_closest[k];
        sum_variance = recompute_proto(n, data, count, closest, p, proto, 
                                       newproto, weight, bouge, distance, 
                                       variance, dist, clear, accumulate,
                                       normalize);
    }
    /* if there are clusters with no pattern, reassign some patterns to them */
    if(n>=p) {
        int zero_cluster = 0;
        for(i=0;i<p;i++)
            if(weight[i]==0)
                zero_cluster++;
        if(zero_cluster) {
            int *counts = (int *) malloc(sizeof(int)*p),
                *bad_cluster = (int *) malloc(sizeof(int)*zero_cluster);

            if(!counts || !bad_cluster)
                run_time_error("Out of memory in k_means");
            zero_cluster = 0;
            for(i=0;i<p;i++) {
                counts[i] = 0;
                if(weight[i]==0)
                    bad_cluster[zero_cluster++] = i;
            }
            for(k=0;k<n;k++) {
                counts[closest[k]] += 1;
                if(counts[closest[k]]>1) {
                    counts[closest[k]] -= 1;
                    closest[k] = bad_cluster[--zero_cluster];
                    counts[closest[k]] += 1;
                    if(!zero_cluster)
                        break;
                }
            }
            sum_variance = recompute_proto(n, data, count, closest, p, proto, 
                                           newproto, weight, bouge, distance, 
                                           variance, dist, clear, accumulate,
                                           normalize);
            free(bad_cluster);
            free(counts);
        }
    }

    /* clean up */
    for (k=0; k<n; k++)
        assign[k] = label[closest[k]];
    for (i=0; i<p; i++) 
        variance[i] /= pow(weight[i], var_norm_exp);

    free(bouge);
    free(distance);
    free(old_closest);
    free(closest);

    return sum_variance;
}

/******************************************************************************
 *
 *  Find the closest prototype cl in <proto> to each sample k in <data> and 
 *  put its index (cl) in <closest>[k].  If the label is different from the 
 *  previous value, set flag for all the affected prototype in <bouge>.
 *  On input, distance between data[k] and proto[closest[k]] is stored in
 *  distance[k] (this is an optimization which saves the computatation of that
 *  distance).
 *
 *  data: 	array of pointers to the sample structures (input)
 *  n:		how many samples (input)
 *  proto:	array of pointers to the cluster center (input)
 *  p: 		how many cluster center (input)
 *  closest:    array of size n of int containing for each sample the closest
 *                  cluster center (input, output)
 *  distance:   array of size n of float containing for each sample the 
 *                  distance to the closest cluster center (input)
 *  dist:	pointer to the distance function (input)
 *  bouge:	array of size p of char.  Flag is 1 for all cluster center 
 * 		    which have a different sample assignement (output)
 *
******************************************************************************/
void find_closest(void *(data[]), int n, void *(proto[]), int p, int *closest,
                  float *distance, float (*dist) (void *, void *), char *bouge)
{
    int i,k;

    for (i=0; i<p; i++) 
        bouge[i] = 0;
    
    for (k=0; k<n; k++) {
        int cl = closest[k];
        float d, mindist=distance[k];
        
        /* find closest cluster center */
        for (i=0; i<p; i++) {
            d = (i==closest[k]) ? distance[k] : dist(data[k], proto[i]);
            if (d<mindist) {
                cl=i;
                mindist=d; 
            } 
        }
        
        /* identify which prototypes have to move */
        if(cl != closest[k]) {
            bouge[cl] = 1;
            bouge[closest[k]] = 1;
            closest[k] = cl;
        }
    }
}

/******************************************************************************
 *
 *  recompute prototypes which have moved, and variance
 *
 *  n:		how many samples (input)
 *  data: 	array of pointers to the sample structures (input)
 *  count: 	array of floats (weighting the samples in data)
 *  closest:    array of size n of int containing for each sample the closest
 *                  cluster center (input)
 *  p: 		how many cluster center (input)
 *  proto:	array of p pointers to the cluster center (input, output)
 *  newproto:	array of p pointers to the new cluster center (work space)
 *  weight:     array of p float containing weight of each cluster (output)
 *  bouge:	array of p char.  Flag is 1 for all cluster center which need
 * 		    to be recomputed (input)
 *  distance:   array of size n of float containing for each sample the 
 *                  distance to the closest cluster center (output)
 *  variance:   variance associated to each cluster
 *  dist:	pointer to the distance function (input)
 *  clear:	pointer to the clear function (input)
 *  accumulate:	pointer to the accumulate function (input)
 *  normalize:	pointer to the normalize function (input)
 *
 ******************************************************************************/
float recompute_proto(int n, void **data, float *count, int *closest, int p,
                      void **proto, void **newproto, float *weight, char *bouge,
                      float *distance, float *variance, 
                      float (*dist) (void *, void *), void (*clear) (void*),
                      void (*accumulate) (void *, float, void *),
                      char (*normalize) (void *,  void *, float))
{
    int i,k, cl;
    float sum_variance;
    
    /* reset prototype which will move */
    for (i=0; i<p; i++) {
        weight[i] = 0;
        if(bouge[i]) {
            variance[i]=0;
            clear(newproto[i]);
        }
    }

    /* accumulate samples in each of the prototype which will move */
    for (k=0; k<n; k++) {
        cl = closest[k];
        weight[cl] += count[k];
        if(bouge[cl]) 
            accumulate(data[k], count[k], newproto[cl]);
    }

    /* move prototypes */
    for (i=0; i<p; i++)
        if(weight[i] != 0 && bouge[i])
            normalize(newproto[i], proto[i], weight[i]);
    
    /* recompute variance of all the prototype which have move */
    for (k=0; k<n; k++) {
        int cl = closest[k];
        if(bouge[cl]) {
            distance[k] = dist(data[k], proto[cl]);
            variance[cl] += count[k] * distance[k];
        }
    }
    
    /* compute global variance */
    sum_variance = 0;
    for (i=0; i<p; i++)
        sum_variance += variance[i]/* / weight[i] */;

    return sum_variance;
}

/******************************************************************************
 *
 *  This was a crazy idea I had.  Call normalize more often to get a 
 *  stochastic (online) update of the prototype rather than batch.
 *  Unfortunately, the speed up is not worth it.
 *
 *  n:		how many samples (input)
 *  data: 	array of pointers to the sample structures (input)
 *  count: 	array of floats (weighting the samples in data)
 *  closest:    array of size n of int containing for each sample the closest
 *                  cluster center (input)
 *  p: 		how many cluster center (input)
 *  proto:	array of p pointers to the cluster center (input, output)
 *  newproto:	array of p pointers to the new cluster center (work space)
 *  weight:     array of p float containing weight of each cluster (output)
 *  bouge:	array of p char.  Flag is 1 for all cluster center which need
 * 		    to be recomputed (input)
 *  distance:   array of size n of float containing for each sample the 
 *                  distance to the closest cluster center (output)
 *  variance:   variance associated to each cluster
 *  dist:	pointer to the distance function (input)
 *  clear:	pointer to the clear function (input)
 *  accumulate:	pointer to the accumulate function (input)
 *  normalize:	pointer to the normalize function (input)
 *
 ******************************************************************************/
float on_line_first(int n, void **data, float *count, int *closest, int p,
                   void **proto, void **newproto, float *weight, char *bouge,
                   float *distance, float *variance, 
                   float (*dist) (void *, void *), void (*clear) (void*),
                   void (*accumulate) (void *, float, void *),
                   char (*normalize) (void *,  void *, float))
{
    int i, k, *updates, freq_update = 10, boo = 0;
    float sum_variance;

    /* reset prototypes */
    for (i=0; i<p; i++) {
        weight[i] = 0;
        variance[i]=0;
        clear(newproto[i]);
    }

    for (k=0; k<n; k++) {
        int cl = closest[k];
        float d, mindist=distance[k];
        float factor = 1;
        
        /* find closest cluster center */
        for (i=0; i<p; i++) {
            d = (i==closest[k]) ? distance[k] : dist(data[k], proto[i]);
            if (d<mindist) {
                cl=i;
                mindist=d; 
            } 
        }
        accumulate(data[k], count[k]*factor, newproto[cl]);
        weight[cl] += count[k]*factor;
        closest[k] = cl;
        boo++;
        if((boo % freq_update) == 0) {
            for(i=0;i<p;i++)
                if(weight[i]!= 0)
                    normalize(newproto[i], proto[i], weight[i]);
        }
    }
    
    /* finish the job moving prototypes */
    for(i=0;i<p;i++)
        if(weight[i]!= 0)
            normalize(newproto[i], proto[i], weight[i]);
    
    /* recompute variance of all the prototype which have move */
    for (k=0; k<n; k++) {
        int cl = closest[k];
        distance[k] = dist(data[k], proto[cl]);
        variance[cl] += count[k] * distance[k];
    }
    
    /* compute global variance */
    sum_variance = 0;
    for (i=0; i<p; i++)
        sum_variance += variance[i];

    return sum_variance;
}

/******************************************************************************
 *
 *  Find the cluster leaf with maximum variance.  Assumes every nodes has
 *	its variance computed.
 *
 ******************************************************************************/
/*
cluster_type *find_max_variance_node(cluster_type *cluster)
{
    cluster_type *child1, *child2;
    
    if(!(cluster->child1))
        return cluster;
    child1 = find_max_variance_node(cluster->child1);
    child2 = find_max_variance_node(cluster->child2);
    return (child1->variance > child2->variance) ? child1 : child2;
}
*/
cluster_type *find_max_variance_node(cluster_type *cluster)
{
    cluster_type *pt;

    for(pt = cluster; pt->child1;)
        if(pt->max_variance == pt->child1->max_variance)
            pt = pt->child1;
        else
            pt = pt->child2;
    return pt;
}

/******************************************************************************
 *
 * Return a new cluster, for n pattern.
 *
 ******************************************************************************/
cluster_type *new_cluster(int n)
{
    cluster_type *nc;

    nc = (struct cluster_type *) malloc(sizeof(struct cluster_type));
    if(!nc)
        error("new_cluster: not enough memory");

    nc->n = n;
    nc->sample_index = (void **) malloc(sizeof(void *) * n);
    nc->sample_weight = (float *) malloc(sizeof(float) * n);
    nc->labels = (int *) malloc(sizeof(int) * n);

    if(!(nc->sample_index) || !(nc->sample_weight) || !(nc->labels))
        error("new_cluster: not enough memory");

    nc->parent = 0;
    nc->child1 = 0;
    nc->child2 = 0;
    nc->cluster = 0;
    nc->variance = -1;
    nc->max_variance = -1;
    nc->split = -1;
    nc->weight = 0;

    return nc;
}

/******************************************************************************
 *
 *  top_cluster
 *
 ******************************************************************************/
cluster_type *top_cluster(
                          /* points on an array of pointers to the samples */
                          void **sample_index,

                          /* points an array of weights to these samples */
                          float *sample_weight,

                          int n, 		/* How many samples? */
                          void *cluster_center,	/* space for cluster center */
                          void *temp_cluster,	/* temp space for cluster */
                          
                          /* This function initialize the clusters with the 
                             samples.  For instance, it set each cluster to 
                             some randomly selected samples */
                          void (*init_cluster)(void **, float *, int,
                                               void **, int),
                          /* ptr to distance function between vectors */
                          float (*dist) (void *, void *), 
                          /* clear the proto */
                          void (*clear) (void*), 
                          /* accumulates samples in the proto */
                          void (*accumulate) (void *, float, void *),
                          /* normalization */
                          char (*normalize) (void *,  void *, float),
                          /* put the norm factor for variance to that power */
                          double var_norm_exp	
                          )
{
    cluster_type *cluster;
    float cur_variance = -1, weight = 1;
    int labels[2] = {0,1}, i;

    CHECK_NULL_POINTER(top_cluster,sample_index);
    CHECK_NULL_POINTER(top_cluster,sample_weight);
    CHECK_NULL_POINTER(top_cluster,cluster_center);
    CHECK_NULL_POINTER(top_cluster,temp_cluster);
    CHECK_NULL_POINTER(top_cluster,init_cluster);
    CHECK_NULL_POINTER(top_cluster,dist);
    CHECK_NULL_POINTER(top_cluster,clear);
    CHECK_NULL_POINTER(top_cluster,accumulate);
    CHECK_NULL_POINTER(top_cluster,normalize);

    /* initialize top node */
    cluster = new_cluster(n);
    cluster->current_split = 1;
    cluster->label = 0;
    cluster->current_label = 1;

    cluster->cluster = cluster_center;

    /* At this point, these are copied, but it may not be necessary.  
       Hopefully, it is negligible compared to k_means, even with 1 cluster */
    for(i=0;i<n;i++) {
        cluster->sample_index[i] = sample_index[i];
        cluster->sample_weight[i] = sample_weight[i];
        cluster->labels[i] = i;
    }

    init_cluster(cluster->sample_index, &weight, cluster->n,
                 &cluster_center, 1);
    k_means(dist, clear, accumulate, normalize,
            cluster->n, cluster->sample_index, 
            cluster->sample_weight, cluster->labels,
            1, &cluster_center, &temp_cluster,
            labels, &(cluster->weight),
            &(cluster->variance), var_norm_exp);

    cluster->max_variance = cluster->variance;
    return cluster;
}

/******************************************************************************
 *
 *  hcluster_build
 *
 ******************************************************************************/
float cluster_split(
                    cluster_type *top_cluster,/* Build from this cluster on*/
                    cluster_type *cluster,/* Build from this cluster on*/                     
                    void *cluster_center1,	/* space for cluster center1 */
                    void *cluster_center2,	/* space for cluster center2 */
                    void *temp_cluster1,	/* temp space for cluster1 */
                    void *temp_cluster2,	/* temp space for cluster2 */
                    
                    /* This function initialize the clusters with the 
                       samples.  For instance, it set each cluster to 
                       some randomly selected samples */
                    void (*init_cluster)(void **sample_index, 
                                         float *weight, int n,
                                         void **cluster, int p),
                    
                    /* ptr to distance function between vectors */
                    float (*dist) (void *, void *), 
                    
                    /* clear the proto */
                    void (*clear) (void*), 
                    
                    /* accumulates samples in the proto */
                    void (*accumulate) (void *, float, void *),
                    
                    /* normalization */
                    char (*normalize) (void *,  void *, float),
                    /* put the norm factor for variance to that power */
                    double var_norm_exp	
                    )
{
    cluster_type *child1, *child2, *pt;
    void *cluster_array[2], *temp_cluster_array[2];
    float cur_variance = -1, weights[2], variance[2];
    int allocated_leaf_cluster = 1, labels[2] = {0,1};
    int count1, count2, i;

    CHECK_NULL_POINTER(cluster_split,top_cluster);
    CHECK_NULL_POINTER(cluster_split,cluster);
    CHECK_NULL_POINTER(cluster_split,cluster_center1);
    CHECK_NULL_POINTER(cluster_split,cluster_center2);
    CHECK_NULL_POINTER(cluster_split,temp_cluster1);
    CHECK_NULL_POINTER(cluster_split,temp_cluster2);
    CHECK_NULL_POINTER(cluster_split,init_cluster);
    CHECK_NULL_POINTER(cluster_split,dist);
    CHECK_NULL_POINTER(cluster_split,clear);
    CHECK_NULL_POINTER(cluster_split,accumulate);
    CHECK_NULL_POINTER(cluster_split,normalize);

    temp_cluster_array[0] = temp_cluster1;
    temp_cluster_array[1] = temp_cluster2;
    cluster_array[0] = cluster_center1;
    cluster_array[1] = cluster_center2;

    if(cluster->variance == 0)
        return 0;
    cluster->split = top_cluster->current_split++;


    init_cluster(cluster->sample_index, cluster->sample_weight, cluster->n,
                 cluster_array, 2);

    k_means(dist, clear, accumulate, normalize, cluster->n, 
            cluster->sample_index, cluster->sample_weight, cluster->labels,
            2, cluster_array, temp_cluster_array, labels, weights,
            variance, var_norm_exp);

    count1 = 0;
    count2 = 0;
    for(i=0;i<cluster->n;i++)
        if(cluster->labels[i] == 0)
            count1++;
        else
            count2++;

    if(count1 == 0 || count2 == 0) {
        run_time_error("What is going on? cluster with zero sample!?");
    }

    child1 = new_cluster(count1);
    child1->label = top_cluster->current_label++;
    child2 = new_cluster(count2);
    child2->label = top_cluster->current_label++;
    child1->cluster = cluster_array[0];
    child2->cluster = cluster_array[1];
    child1->variance = variance[0];
    child2->variance = variance[1];
    child1->max_variance = variance[0];
    child2->max_variance = variance[1];
    child1->parent = cluster;
    child2->parent = cluster;
    
    count1 = 0;
    count2 = 0;
    for(i=0;i<cluster->n;i++)
        if(cluster->labels[i] == 0) {
            child1->sample_index[count1] = cluster->sample_index[i];
            child1->sample_weight[count1] = cluster->sample_weight[i];
            count1++;
        } else {
            child2->sample_index[count2] = cluster->sample_index[i];
            child2->sample_weight[count2] = cluster->sample_weight[i];
            count2++;
        }

    child1->weight = weights[0];
    child2->weight = weights[1];
    cluster->child1 = child1;
    cluster->child2 = child2;

    for(pt = cluster; pt; pt = pt->parent)
        pt->max_variance = max(pt->child1->max_variance, pt->child2->max_variance);
    
    return cluster->max_variance;
}

float hcluster_build(
		     cluster_type *top_cluster,/* Build from this cluster on*/
                     
                     void *cluster_center1,	/* space for cluster center1 */
                     void *cluster_center2,	/* space for cluster center2 */
                     void *temp_cluster1,	/* temp space for cluster1 */
                     void *temp_cluster2,	/* temp space for cluster2 */
		     
		     /* This function initialize the clusters with the 
			samples.  For instance, it set each cluster to 
			some randomly selected samples */
		     void (*init_cluster)(void **sample_index, 
                                          float *weight, int n,
                                          void **cluster, int p),
                     
		     /* ptr to distance function between vectors */
		     float (*dist) (void *, void *), 
                     
		     /* clear the proto */
		     void (*clear) (void*), 

		     /* accumulates samples in the proto */
		     void (*accumulate) (void *, float, void *),
                     
		     /* normalization */
		     char (*normalize) (void *,  void *, float),
                     /* put the norm factor for variance to that power */
                     double var_norm_exp	
                     )
{
    cluster_type *cluster;
    
    CHECK_NULL_POINTER(cluster_build,top_cluster);
    CHECK_NULL_POINTER(cluster_build,cluster_center1);
    CHECK_NULL_POINTER(cluster_build,cluster_center2);
    CHECK_NULL_POINTER(cluster_build,temp_cluster1);
    CHECK_NULL_POINTER(cluster_build,temp_cluster2);
    CHECK_NULL_POINTER(cluster_build,init_cluster);
    CHECK_NULL_POINTER(cluster_build,dist);
    CHECK_NULL_POINTER(cluster_build,clear);
    CHECK_NULL_POINTER(cluster_build,accumulate);
    CHECK_NULL_POINTER(cluster_build,normalize);

    /* find which node to split */
    cluster = find_max_variance_node(top_cluster);
    if(cluster->variance == 0)
        return 0;
    
    cluster_split(top_cluster, cluster, cluster_center1, cluster_center2,
                  temp_cluster1, temp_cluster2, init_cluster, dist, clear,
                  accumulate, normalize, var_norm_exp);
    
    return find_max_variance_node(top_cluster)->variance;
}

/******************************************************************************
 *
 *   find_cluster_slice(cluster_type *cluster, cluster_type **cluster_array,
 *	int level, int *local_label, int leaf_number)
 *
 *	Input:
 *		cluster: has a current cluster pointer
 *		level:   What slice of the tree are we interested in
 *		leaf_number: how many cluster in the slice have we seen.
 *	Output:
 *		cluster_array: cluster_array[*leaf_number] is updated with
 *			with the leaf_number-th cluster of the slice.
 *		local_label: Give a label (as an index in cluster_array) for
 *			each of the sample of the node cluster.
 *		leaf_number: how many cluster in the slice have we seen.
 *
******************************************************************************/
int find_cluster_slice(cluster_type *cluster, cluster_type **cluster_array,
                       int level, int *local_labels, int leaf_number)
{
    CHECK_NULL_POINTER(find_cluster_slice,cluster);
    CHECK_NULL_POINTER(find_cluster_slice,cluster_array);
    CHECK_NULL_POINTER(find_cluster_slice,local_labels);

    /* if leaf, or level of children higher than level */
    if(cluster->split >= level || cluster->child1 == 0) {
        int i;

        for(i=0;i<cluster->n;i++)
            local_labels[i] = leaf_number;
        cluster_array[leaf_number] = cluster;
        return leaf_number+1;
    } else {
        int *local_label1 = (int *) alloca(sizeof(int) * cluster->child1->n),
            *local_label2 = (int *) alloca(sizeof(int) * cluster->child2->n),
            count1 = 0, count2 = 0, i;
        
        leaf_number = find_cluster_slice(cluster->child1, cluster_array, level,
                                         local_label1, leaf_number);
        leaf_number = find_cluster_slice(cluster->child2, cluster_array, level,
                                         local_label2, leaf_number);
        for(i=0;i<cluster->n;i++)
            if(cluster->labels[i] == 0) 
                local_labels[i] = local_label1[count1++];
            else
                local_labels[i] = local_label2[count2++];
        return leaf_number;
    }
}

/******************************************************************************
 *
 * free_cluster_node(cluster)
 *     Free everyting allocated by hcluster_build starting at <cluster>.
 *
 ******************************************************************************/
free_cluster_node(cluster_type *cluster, void (*free_sample_ptr) (void *) )
{
    CHECK_NULL_POINTER(find_cluster_slice,cluster);

    if(cluster->child1) {
        free_cluster_node(cluster->child1, free_sample_ptr);
        free_cluster_node(cluster->child2, free_sample_ptr);
    }

    free(cluster->labels);
    free(cluster->sample_index);
    free(cluster->sample_weight);
    free_sample_ptr(cluster->cluster);
    free(cluster);
}
