#include <time.h> 
#include <stdio.h>
#include <alsa/asoundlib.h>
#include <math.h>
 
static char *device = "default";        
static snd_pcm_format_t format = SND_PCM_FORMAT_S16;    
static unsigned int rate = 44100;       
static snd_output_t *output = NULL;
static int sample_size = 44100 * 2; // 1 second sample at 44100 hz, where each sample takes 2 bytes
static int maxFreq = 880; // x axis in drawing_sruface translation bounds
static int minFreq = 110;
static int steps = 6; // y axis in drawing_surface

static double get_sinusoid(double freq, double phase)
{
  return sin(2*M_PI*freq*phase);
}


static void populate_buff(unsigned char *samples, double freq, int partials)
{
  int res, d;
  double loc, f;
  unsigned char *ptr = samples;
  
  double chordstep = 0.25;
  int chorddegree = (partials+1);
  unsigned int maxval = (1 << 15) - 1;
  // maxval /= chorddegree;

  // value we need to divide the signal by this partial sum (sum of 1/n**2 for n partials)
  double partialsum = 1;
  for (int j = 0; j < partials; j++)
    {
      partialsum += 1/pow(2, j);
      printf("%f\n", freq*pow(2, j+1));
    }
  
  for (int i = 0; i < sample_size/2; i++)
    {
      res = 0;
      //chords
      /*
      d = 0;
      f = freq;
      
      while (d++ < chorddegree)
	{
	  //poor way of implementing b/c we need to do all these calculations for each phase of sample
	  // better to calc the d (degree of chord) frequencies and just look up for each phase of sample
	  loc = (double)i/sample_size*2;
	  f = f * (1.0 + (double)chordstep*d);
	  res = res + get_sinusoid(f, loc)*maxval;
	}

      
      */


      // partials
      /*
      loc = (double)i/sample_size*2;
      res = get_sinusoid(freq, loc)*maxval/partialsum;
      
      for (int j = 0; j < partials; j++)
	{
	  if (freq*pow(2,(j+1)) > rate/2) break;
							      
	  res += get_sinusoid(freq*pow(2,(j+1)), loc)*maxval/pow(2, (j+1))/partialsum;
	}
      */

      
      // sound shape
      //      res *= sin(loc*M_PI)/2;
      //      res *= exp(-loc);

      
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
