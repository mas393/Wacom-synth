#include <time.h> 
#include <stdio.h>
#include <alsa/asoundlib.h>
#include <math.h>
 
static char *device = "default";        
static snd_pcm_format_t format = SND_PCM_FORMAT_S16;    
static unsigned int rate = 44100;       
static snd_output_t *output = NULL;
static int sample_size = 44100 * 2; // 1 second sample at 44100 hz, where each sample takes 2 bytes

static void populate_buff(unsigned char *samples, double freq, int partials)
{
  int res;
  double loc;
  unsigned char *ptr = samples;
  unsigned int maxval = (1 << 15) - 1;
  for (int i = 0; i < sample_size/2; i++)
    {
      loc = (double)i/sample_size*2;
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
	  //singals occupy 2 bytes therefore need to shift for little-endian-ness
	  *(ptr + j) = (res >> j * 8) & 0xff;	  
	}

      ptr = ptr + 2;

    }
}

/*
 *   Transfer method - write only
 */
 
static int write_loop(snd_pcm_t *handle,
		      unsigned char *samples,
		      double freq, int partials)
{
    int err;
    populate_buff(samples, freq, partials);
    err = snd_pcm_writei(handle, samples, sample_size/2);

}
 
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
    unsigned char *samples;
    double freq;
    int partials;

    translate_coords(maxx, maxy, x, y, &freq, &partials);
 
    snd_pcm_hw_params_alloca(&hwparams);
 
    snd_output_stdio_attach(&output, stdout, 0);

    printf("Playback device is %s\n", device);
    printf("Stream parameters are %uHz, %s,\n", rate, snd_pcm_format_name(format));
    printf("Sine wave rate is %.4fHz\n", freq);
 
    snd_pcm_open(&handle, device, SND_PCM_STREAM_PLAYBACK, 0);
    snd_pcm_hw_params_any(handle, hwparams);
    snd_pcm_hw_params_set_format(handle, hwparams, format);
    snd_pcm_hw_params_set_rate_near(handle, hwparams, &rate, 0);
    snd_pcm_hw_params(handle, hwparams);
    
    samples = malloc(sample_size);
    write_loop(handle, samples, freq, partials);
    
    free(samples);
    //might want to open handle when app opens and close it when app closes to avoid the click sound
    sleep(1);
    snd_pcm_close(handle);
    return 0;
}
