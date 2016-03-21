#ifndef BENCHMARK_H
#define BENCHMARK_H

#include <stdio.h>
#include <stdlib.h>
#include <hwloc.h>

int  roofline_lib_init(void);
void roofline_lib_finalize(void);

/************************************ Benchmark memory and fpu  **********************************/

/***********************************  BENCHMARK INPUT  *******************************************/
struct roofline_sample_in{
    /* All sample type specific data */
    long loop_repeat;    /* Make the roofline longer if you use an external tool to sample */
    double * stream;          /* The buffer to stream */
    size_t stream_size;       /* The total size to stream */
};

void print_roofline_sample_input(struct roofline_sample_in * in);

/***********************************  BENCHMARK OUTPUT *******************************************/
struct roofline_sample_out{
    /* All sample type specific data */
    uint64_t ts_start;       /* Timestamp in cycles where the roofline started */
    uint64_t ts_end;         /* Timestamp in cycles where the roofline ended */
    uint64_t instructions;   /* The number of instructions */
    uint64_t bytes;          /* The amount of bytes transfered */
    uint64_t flops;          /* The amount of flops computed */
};

void roofline_output_clear(struct roofline_sample_out * out);
void roofline_output_accumulate(struct roofline_sample_out *, struct roofline_sample_out *);
void print_roofline_sample_output(struct roofline_sample_out * out);

/***********************************  BENCHMARK FUNCTIONS ****************************************/
#define ROOFLINE_LOAD 0
#define ROOFLINE_STORE 1
#define ROOFLINE_N_SAMPLES 16

void roofline_fpeak    (FILE * output);
void roofline_bandwidth(FILE * output, hwloc_obj_t memory, int type);
void roofline_oi       (FILE * output, hwloc_obj_t memory, int type, double oi);

/******************************************** Progress Bar ***************************************/
struct roofline_progress_bar{
    char * info;
    size_t begin;
    size_t current;
    size_t end;
};

extern struct roofline_progress_bar progress_bar;
void roofline_progress_set  (struct roofline_progress_bar * bar, char * info, size_t low, size_t curr, size_t up);
void roofline_progress_print(struct roofline_progress_bar * bar);
void roofline_progress_clean(void);

/*************************************  Statistical sampling *************************************/
#define BENCHMARK_REPEAT 16
#ifndef BENCHMARK_MIN_DUR
#define BENCHMARK_MIN_DUR 1 /* milliseconds */
#endif

int    comp_roofline_throughput(void * a, void * b);
int    roofline_output_min(struct roofline_sample_out * samples, size_t n);
int    roofline_output_max(struct roofline_sample_out * samples, size_t n);
int    roofline_output_median(struct roofline_sample_out * samples, size_t n);
double roofline_output_sd(struct roofline_sample_out * samples, unsigned n);
double roofline_repeat_bench(void (* bench_fun)(struct roofline_sample_in *, struct roofline_sample_out *), struct roofline_sample_in * in, struct roofline_sample_out * out, int (* bench_stat)(struct roofline_sample_out * , size_t));
long roofline_autoset_loop_repeat(void (* bench_fun)(struct roofline_sample_in *, struct roofline_sample_out *), struct roofline_sample_in * in, long ms_dur);

/******************************************* Hardware locality ***********************************/
extern hwloc_topology_t topology; /* The current machine topology */
extern size_t alignement;         /* The level 1 cache line size */
extern size_t LLC_size;           /* The last level cache size */
extern float  cpu_freq;           /* The cpu frequency defined by BENCHMARK_CPU_FREQ environment or at build time in the same #define directive */

int         roofline_hwloc_objtype_is_cache(hwloc_obj_type_t type);
int         roofline_hwloc_obj_snprintf(hwloc_obj_t obj, char * info_in, size_t n);
int         roofline_hwloc_check_cpu_bind(hwloc_cpuset_t cpuset, int print);
int         roofline_hwloc_check_mem_bind(hwloc_cpuset_t nodeset, int print);
hwloc_obj_t roofline_hwloc_parse_obj(char*);
int         roofline_hwloc_cpubind(hwloc_cpuset_t);
int         roofline_hwloc_membind(hwloc_obj_t);
size_t      roofline_hwloc_get_memory_size(hwloc_obj_t obj);
hwloc_obj_t roofline_hwloc_get_next_memory(hwloc_obj_t obj);
hwloc_obj_t roofline_hwloc_get_previous_memory(hwloc_obj_t obj);
hwloc_obj_t roofline_hwloc_get_instruction_cache(void);
size_t      roofline_hwloc_get_instruction_cache_size(void);

/********************************************* Utils ********************************************/
#define errEXIT(msg) do{fprintf(stderr,msg"\n"); exit(EXIT_FAILURE);} while(0);
#define perrEXIT(msg) do{perror(msg); exit(EXIT_FAILURE);} while(0);
#define roofline_macro_str(x) #x
#define roofline_macro_xstr(x) roofline_macro_str(x)
#define roofline_ABS(x) ((x)>0 ? (x):-(x))
#define roofline_MAX(x, y) ((x)>(y) ? (x):(y))
#define roofline_MIN(x, y) ((x)<(y) ? (x):(y))
#define roofline_BOUND(a, x, y) (a>x ? (a<y?a:y) : x)
#define roofline_alloc(ptr,size) do{if(!(ptr=malloc(size))){perrEXIT("malloc");}} while(0)
#define roofline_realloc(ptr,size,max_size)				\
    do{									\
	while(size>=max_size){max_size*=2;}				\
	if((ptr = realloc(ptr,sizeof(*ptr)*max_size)) == NULL) perrEXIT("realloc"); \
    } while(0)

const char * roofline_type_str(int type);
void   roofline_print_header(FILE * output, const char * append);
void   roofline_print_sample(FILE * output, hwloc_obj_t obj, struct roofline_sample_out * sample_out, double sd, unsigned n_threads, const char * append);

/**
 * Compute a logarithmic array of sizes
 * @arg start: The first element of the array. 
 * @arg end: The last element of the array. end
 * @arg n(in/out): the number of element in array.
 * @return A logarithmic array of sizes starting with start. Sizes are truncated to the closest integer value.
 **/
size_t * roofline_log_array(size_t start, size_t end, int * n);

#endif /* BENCHMARK_H */
