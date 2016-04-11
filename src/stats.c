#include <sys/time.h>
#include <math.h>
#include "roofline.h"

unsigned roofline_PGCD(unsigned a, unsigned b){
    unsigned max,min, r;
    max = roofline_MAX(a,b);
    min = roofline_MIN(a,b);
    while((r=max%min)!=0){max = min; min = r;}
    return min;
}

unsigned roofline_PPCM(unsigned a, unsigned b){
    return a*b/roofline_PGCD(a,b);
}

double roofline_output_sd(struct roofline_sample_out * out, unsigned n){
    unsigned i;
    double mean = 0, M2 = 0, delta = 0, throughput;
    for(i=0;i<n;i++){
	throughput = (double)(out[i].instructions) / (double)(out[i].ts_end - out[i].ts_start);
	delta = throughput-mean;
        mean += delta/(i+1);
	M2 += delta*(throughput - mean);
    }
    if(n < 2)
        return 0;
    else
        return sqrt(M2 / (n-1));
}

int comp_roofline_throughput(void * a, void * b){
    struct roofline_sample_out a_out, b_out;
    float val_a, val_b;
    a_out = (*(struct roofline_sample_out *)a);
    b_out = (*(struct roofline_sample_out *)b);
    if(a_out.ts_end == a_out .ts_start)
	val_a = 0;
    else
	val_a = a_out.instructions / (float)(a_out.ts_end - a_out .ts_start);
    if(b_out.ts_end == b_out .ts_start)
	val_b = 0;
    else
	val_b = b_out.instructions / (float)(b_out.ts_end - b_out .ts_start);
    if( val_a < val_b )
	return -1;
    else if(val_a > val_b)
	return 1;
    else
	return 0;
}

int roofline_output_median(struct roofline_sample_out * samples, size_t n){
    qsort(samples, n, sizeof(*samples), (__compar_fn_t)comp_roofline_throughput);
    return (int)(n/2);
}

float roofline_output_mean(struct roofline_sample_out * samples, size_t n){
    float sum = 0;
    size_t i = n;
    while(i--)
	sum+=( (float)(samples[i].instructions) / (float)(samples[i].ts_end-samples[i].ts_start) );
    return sum/n;
}

int roofline_output_max(struct roofline_sample_out * samples, size_t n){
    int ret;
    struct roofline_sample_out max;

    ret = 0;
    max = samples[--n];

    while(n--){
	if(comp_roofline_throughput(&max,&samples[n]) == -1){ 
	    max = samples[n];
	    ret = n;
	}
    }
    return ret;
}

int roofline_output_min(struct roofline_sample_out * samples, size_t n){
    int ret;
    struct roofline_sample_out min;

    ret = 0;
    min = samples[--n];
    while(n--){
	if(comp_roofline_throughput(&min,&samples[n]) == 1){ 
	    min = samples[n];
	    ret = n;
	}
    }
    return ret;
}

long roofline_autoset_loop_repeat(void (* bench_fun)(struct roofline_sample_in *, struct roofline_sample_out *), struct roofline_sample_in * in, long ms_dur, unsigned long min_rep){
    float mul;
    long tv_ms;
    struct roofline_sample_out out;

    in->loop_repeat = 1; 
    mul = 0;
    tv_ms = 0;
    while(tv_ms < ms_dur){
	roofline_output_clear(&out);
	bench_fun(in,&out);
	tv_ms = (out.ts_end-out.ts_start)*1e3/cpu_freq;
	if(tv_ms==0){
	    in->loop_repeat *= 2;
	}

	else if( tv_ms < ms_dur ){
	    mul = (float)ms_dur/(float)tv_ms;
	    if((long)mul <= 1)
		in->loop_repeat += 1;
	    else
		in->loop_repeat *= mul;
	}
    }

    in->loop_repeat = roofline_MAX(min_rep, in->loop_repeat);
    return tv_ms;
}


double
roofline_repeat_bench(void (* bench_fun)(struct roofline_sample_in *, struct roofline_sample_out *), 
		      struct roofline_sample_in * in, struct roofline_sample_out * out, 
		      int (* bench_stat)(struct roofline_sample_out * , size_t))
{
    unsigned i;
    struct roofline_sample_out * samples; 
    double sd; 

    roofline_alloc(samples,sizeof(*samples)*BENCHMARK_REPEAT);
    for(i=0;i<BENCHMARK_REPEAT;i++){
	roofline_output_clear(&(samples[i]));
	bench_fun(in,&samples[i]);
    }    
    /* sort results */
    *out = samples[bench_stat(samples, i)];
    sd = roofline_output_sd(samples,i);
    free(samples);
    return sd;
}

