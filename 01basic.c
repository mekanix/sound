#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/soundcard.h>
#include <unistd.h>


#ifndef SAMPLE_SIZE
#define SAMPLE_SIZE 32
#endif

#if SAMPLE_SIZE == 32
typedef int32_t sample_t;
int format = AFMT_S32_NE; /* Signed 32bit native endian format */
#elif SAMPLE_SIZE == 24
typedef int24_t sample_t;
int format = AFMT_S24_NE; /* Signed 24bit native endian format */
#elif SAMPLE_SIZE == 16
typedef int16_t sample_t;
int format = AFMT_S16_NE; /* Signed 16bit native endian format */
#elif SAMPLE_SIZE == 8
typedef int8_t sample_t;
int format = AFMT_S8_NE; /* Signed 8bit native endian format */
#endif


/* Minimal configuration for OSS
 * For real world applications, this structure will probably contain many
 * more fields
 */
typedef struct config
{
  char *device;
  int channels;
  int fd;
  int format;
  int frag;
  int fragSize;
  int samplerate;
  int sampleSize;
  int nsamples;
} config_t;


/* Error state is indicated by value=-1 in which case application exits
 * with error
 */
static void checkError(const int value, const char *message)
{
  if (value == -1)
  {
    fprintf(stderr, "OSS error: %s %s\n", message, strerror(errno));
    exit(1);
  }
}



/* Calculate frag by giving it minimal size of buffer */
static int size2frag(int x)
{
  int frag = 0;
  while ((1 << frag) < x) { ++frag; }
  return frag;
}


int main(int argc, char **argv)
{
  config_t config = {
    .device = "/dev/dsp",
    .channels = 2,
    .format = format,
    .frag = 10,
    .samplerate = 48000,
    .sampleSize = sizeof(sample_t),
  };
  int devcaps;
  int error;
  int tmp;
  oss_audioinfo ai;

  /* Open the device for read and write */
  config.fd = open(config.device, O_RDWR);
  checkError(config.fd, "open");

  /* Get device information */
  ai.dev = -1;
  error = ioctl(config.fd, SNDCTL_ENGINEINFO, &ai);
  checkError(error, "SNDCTL_ENGINEINFO");
  printf("min_channels: %d\n", ai.min_channels);
  printf("max_channels: %d\n", ai.max_channels);
  printf("latency: %d\n", ai.latency);
  printf("handle: %s\n", ai.handle);
  if (ai.min_rate > config.samplerate || config.samplerate > ai.max_rate)
  {
    fprintf(stderr, "%s doesn't support chosen ", config.device);
    fprintf(stderr, "samplerate of %dHz!\n", config.samplerate);
    exit(1);
  }

  error = ioctl(config.fd, SNDCTL_DSP_GETCAPS, &devcaps);
  checkError(error, "SNDCTL_DSP_GETCAPS");

  /* Set number of channels. If number of channels is chosen to the value
   * near the one wanted, save it in config
   */
  tmp = config.channels;
  error = ioctl(config.fd, SNDCTL_DSP_CHANNELS, &tmp);
  checkError(error, "SNDCTL_DSP_CHANNELS");
  if (tmp != config.channels)
  {
    fprintf(stderr, "%s doesn't support chosen ", config.device);
    fprintf(stderr, "channel count of %d", config.channels);
    fprintf(stderr, ", set to %d!\n", tmp);
  }
  config.channels = tmp;

  /* If desired frag is smaller than minimum, based on number of channels
   * and format (size in bits: 8, 16, 24, 32), set that as frag. Buffer size
   * is 2^frag, but the real size of the buffer will be read when the
   * configuration of the device is successfull
   */
  int minFrag = size2frag(config.sampleSize * config.channels);
  if (config.frag < minFrag) { config.frag = minFrag; }
  /* Allocate N fragments of size config.frag. In this case, N = 1 */
  int max_fragments;
  if (argc == 1) { max_fragments = 0; }
  else { max_fragments = atoi(argv[1]); }
  printf("max_fragments: %d\n", max_fragments);

  tmp = (max_fragments << 16) | config.frag;
  error = ioctl(config.fd, SNDCTL_DSP_SETFRAGMENT, &tmp);
  checkError(error, "SNDCTL_DSP_SETFRAGMENT");

  /* Set format, or bit size: 8, 16, 24 or 32 bit sample */
  tmp = config.format;
  error = ioctl(config.fd, SNDCTL_DSP_SETFMT, &tmp);
  checkError(error, "SNDCTL_DSP_SETFMT");
  if (tmp != config.format)
  {
    fprintf(stderr, "%s doesn't support chosen sample format!\n", config.device);
    exit(1);
  }

  /* Most common values for samplerate (in kHz): 44.1, 48, 88.2, 96 */
  tmp = config.samplerate;
  error = ioctl(config.fd, SNDCTL_DSP_SPEED, &tmp);
  checkError(error, "SNDCTL_DSP_SPEED");

  /* When all is set and ready to go, get the size of buffer */
  error = ioctl(config.fd, SNDCTL_DSP_GETBLKSIZE, &config.fragSize);
  checkError(error, "SNDCTL_DSP_GETBLKSIZE");
  config.nsamples = config.fragSize / config.sampleSize / config.channels;

  sample_t ibuf[config.nsamples];
  sample_t obuf[config.nsamples];

  printf("frag: %d, nsamples: %d, fragSize: %d\n", config.frag, config.nsamples, config.fragSize);
  return 0;
}
