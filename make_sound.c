#include <time.h> 
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <alsa/asoundlib.h>
#include <math.h>
 
static char *device = "default";         /* playback device */
static snd_pcm_format_t format = SND_PCM_FORMAT_S16;    /* sample format */
static unsigned int rate = 44100;           /* stream rate */
static unsigned int channels = 1;           /* count of channels */
static unsigned int buffer_time = 500000;       /* ring buffer length in us */
static unsigned int period_time = 100000;       /* period time in us */
//static double freq = 220;               /* sinusoidal wave frequency in Hz */
static int verbose = 0;                 /* verbose flag */
static int resample = 1;                /* enable alsa-lib resampling */
static int period_event = 0;                /* produce poll event after each period */
 
static snd_pcm_sframes_t buffer_size;
static snd_pcm_sframes_t period_size;
static snd_output_t *output = NULL;

static int sample_sec = 1;
static int sample_size = 44100 * 2;

static void populate_buff(signed short *samples, double freq, int partials)
{
  int res;
  double loc;
  unsigned char *ptr = (unsigned char *)samples;
  unsigned int maxval = (1 << 15) - 1;
  for (int i = 0; i < sample_size/2; i++)
    {
      loc = (double)i/sample_size*2;
      //      printf("%f\n", loc);
      double a = 2*M_PI*freq*loc;

      res = sin(a)*maxval;

      // partials
      for (int j = 1; j < partials; j++)
	{
	  a = a*2;
	  res += sin(a)*maxval/pow(2, j)/pow(2, j);
	}

      // sound shape
      res *= sin(loc*M_PI)/2;



      for (int j = 0; j < 2; j++)
	{
	  //something weird if ptr is signed short as opposed to unsigned char
	  *(ptr + j) = (res >> j * 8) & 0xff;	  
	}

      ptr = ptr + 2;

    }
  //  printf("loc %f", loc);
}

static void generate_sine(const snd_pcm_channel_area_t *areas, 
			  snd_pcm_uframes_t offset,
			  int count, double *_phase, double freq, int partials)
{
    static double max_phase = 2. * M_PI;
    double phase = *_phase;
    double step = max_phase*freq/(double)rate;
    //    printf("rate = %d step = %f\n", rate, step); //rate = 44100 step = 0.060346
    unsigned char *samples;
    int steps;
    int format_bits = snd_pcm_format_width(format); // = 16
    unsigned int maxval = (1 << (format_bits - 1)) - 1;
    //    printf("maxval = %d\n", maxval); //maxval = 32767
    int bps = format_bits / 8;  /* bytes per sample */
    //    printf("bytes = %d\n",bps ); //bytes = 2
 
    /* verify and prepare the contents of areas */
    samples = ((unsigned char *)areas[0].addr);
    steps = areas[0].step / 8; // steps=2
    
    while (count-- > 0) {

        int res, i, totaldecay;
      
	
	res = sin(phase)*maxval;
	totaldecay = 0;
	for (int j = 1; j < partials; j++)
	  {
	    double p = phase * pow(2, j);
	    res += sin(p)/pow(2,j)/pow(2,j)*maxval;
	  }
	res = res/2;
	
	  // we are little endian
	for (i = 0; i < bps; i++)
	  *(samples + i) = (res >>  i * 8) & 0xff;

	samples += steps;
        
        phase += step;
        if (phase >= max_phase)
            phase -= max_phase;
	
    }

    *_phase = phase;
}
 
static int set_hwparams(snd_pcm_t *handle,
            snd_pcm_hw_params_t *params,
            snd_pcm_access_t access)
{
    unsigned int rrate;
    snd_pcm_uframes_t size;
    int err, dir;
 
    /* choose all parameters */
    err = snd_pcm_hw_params_any(handle, params);

    /* set hardware resampling */
    err = snd_pcm_hw_params_set_rate_resample(handle, params, resample);

    /* set the interleaved read/write format */
    err = snd_pcm_hw_params_set_access(handle, params, access);

    /* set the sample format */
    err = snd_pcm_hw_params_set_format(handle, params, format);

    /* set the count of channels */
    err = snd_pcm_hw_params_set_channels(handle, params, channels);

    /* set the stream rate */
    rrate = rate;
    err = snd_pcm_hw_params_set_rate_near(handle, params, &rrate, 0);

    /* set the buffer time */
    err = snd_pcm_hw_params_set_buffer_time_near(handle, params, &buffer_time, &dir);

    err = snd_pcm_hw_params_get_buffer_size(params, &size);

    buffer_size = size;
    /* set the period time */
    err = snd_pcm_hw_params_set_period_time_near(handle, params, &period_time, &dir);

    err = snd_pcm_hw_params_get_period_size(params, &size, &dir);

    period_size = size;
    /* write the parameters to device */
    err = snd_pcm_hw_params(handle, params);

    return 0;
}

/*
 *   Transfer method - write only
 */
 
static int write_loop(snd_pcm_t *handle,
		      signed short *samples,
		      snd_pcm_channel_area_t *areas, double freq, int partials)
{
    int err;
    populate_buff(samples, freq, partials);
    err = snd_pcm_writei(handle, samples, sample_size/2);

  /*
    double phase = 0;
    signed short *ptr;
    snd_pcm_sframes_t err;
    int  cptr;
    int i = 0;
    double maxruntime = 1;
    double runtime, prog;
    time_t start, end;
    time(&start);
    time(&end);

    runtime = difftime(end, start);
    while (runtime < maxruntime) {

        generate_sine(areas, 0, period_size, &phase, freq, partials);
        ptr = samples;
        cptr = period_size; //how many samples write to buffer
	printf(" period size %d\n", cptr);
        while (cptr > 0) {
	    err = snd_pcm_writei(handle, ptr, cptr);
	    printf("err %ld \n", err);
	    i++;
            ptr += err;
            cptr -= err;
        }
	time(&end);
	runtime = difftime(end, start);
	printf("%d\n", i);
    }
  */
}
 
struct transfer_method {
    const char *name;
    snd_pcm_access_t access;
    int (*transfer_loop)(snd_pcm_t *handle,
                 signed short *samples,
			 snd_pcm_channel_area_t *areas,
			 double freq, int partials);
};
 
static struct transfer_method transfer_methods[] = {
    { "write", SND_PCM_ACCESS_RW_INTERLEAVED, write_loop },
};
 

static int maxFreq = 880;
static int minFreq = 110;
static int steps = 6;
// need to make the translate function depend on the gui buttons (synth modes)
static void translate_coords(double maxx, double maxy, double x, double y, double *fx, int *fy)
{
  (*fx) = x/maxx * (maxFreq - minFreq) + minFreq;
  (*fy) = y/maxy * steps;
  printf("y %f maxy %f fy %d\n", y, maxy, *fy);
}

int make_sound(double maxx, double maxy, double x, double y)
{
    snd_pcm_t *handle;
    snd_pcm_hw_params_t *hwparams;
    signed short *samples;
    //    unsigned int chn;
    snd_pcm_channel_area_t *areas;
    double freq;
    int partials;

    translate_coords(maxx, maxy, x, y, &freq, &partials);
 
    snd_pcm_hw_params_alloca(&hwparams);
 
    snd_output_stdio_attach(&output, stdout, 0);

    printf("Playback device is %s\n", device);
    printf("Stream parameters are %uHz, %s, %u channels\n", rate, snd_pcm_format_name(format), channels);
    printf("Sine wave rate is %.4fHz\n", freq);
    //    printf("Using transfer method: %s\n", transfer_methods[method].name);
 
    snd_pcm_open(&handle, device, SND_PCM_STREAM_PLAYBACK, 0);

    //    printf("%s", transfer_methods)
    set_hwparams(handle, hwparams, SND_PCM_ACCESS_RW_INTERLEAVED); // transfer_methods[method].access);
    
    //samples = malloc((period_size * channels * snd_pcm_format_physical_width(format)) / 8);
    samples = malloc(sample_size);
    //    areas = calloc(channels, sizeof(snd_pcm_channel_area_t));    
    //    areas[0].addr = samples;
    //    areas[0].first = 0 * snd_pcm_format_physical_width(format);
    //    areas[0].step = channels * snd_pcm_format_physical_width(format);
 
    //    transfer_methods[method].transfer_loop(handle, samples, areas, freq, partials);
    write_loop(handle, samples, areas, freq, partials);
 
    //    free(areas);
    free(samples);
    sleep(1);
    //might want to open handle when app opens and close it when app closes to avoid the click sound
    snd_pcm_close(handle);
    return 0;
}
