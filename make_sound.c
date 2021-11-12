#include <time.h> 
#include <stdio.h>
#include <alsa/asoundlib.h>
#include <math.h>
 
static char *device = "default";        
static snd_pcm_format_t format = SND_PCM_FORMAT_S16;    
static unsigned int rate = 44100;       
static snd_output_t *output = NULL;
static int sample_size = 44100 * 2; // 1 second sample at 44100 hz, where each sample takes 2 bytes
static int maxFreq = 400; // x axis in drawing_sruface translation bounds
static int minFreq = 80;
static int steps = 5; // y axis in drawing_surface

static double get_sinusoid(double freq, double phase)
{
  return sin(2*M_PI*freq*phase);
}

static double decay(int partial)
{
  return pow(2, partial);
}

static double apply_partials(double freq, double phase, int partials)
{
  double res = 0;
  double decaysum = 0;
  for (int p = 0; p <= partials; p++)
    {
      if (freq*pow(2,(p+1)) > rate/2) break; //aliasing occurs over nyquist freq		       
      res += get_sinusoid(freq*(p+1), phase)/decay(p);
      decaysum += 1/decay(p);
    }

  res /= decaysum;
  
  if (res > 1) res = 0; //will produce a terrible sound
  return res;
}

static void populate_buff(unsigned char *samples, double freq, int partials)
{
  int d;
  double res;
  double loc, f;
  unsigned char *ptr = samples;
  
  double chordstep = 0.25;
  int chorddegree = 3;// (partials+1);
  unsigned int maxval = (1 << 15) - 1;
  maxval /= chorddegree;
  
  for (int i = 0; i < sample_size/2; i++)
    {
      res = 0;
      //chords not sounding good

      /* d = 0; */
      /* f = freq; */
      
      /* while (d++ < chorddegree) */
      /* 	{ */
      /* 	  //poor way of implementing b/c we need to do all these calculations for each phase of sample */
      /* 	  // better to calc the d (degree of chord) frequencies and just look up for each phase of sample */
      /* 	  loc = (double)i/sample_size*2; */
      /* 	  f = f * (1.0 + (double)chordstep*d); */
      /* 	  res = res + apply_partials(f, loc, 1)*maxval; */
      /* 	} */
      
      
      loc = (double)i/sample_size*2;
      res = apply_partials(freq, loc, partials);
      res *= maxval;
      
      // sound shape
      //      res *= sin(loc*M_PI)/2;
      res *= exp(-loc);


      
      for (int j = 0; j < 2; j++) *(ptr + j) = ((int)res >> j * 8) & 0xff;
      
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
