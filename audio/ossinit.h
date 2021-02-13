#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/soundcard.h>
#include <unistd.h>


#ifndef SAMPLE_SIZE
#define SAMPLE_SIZE 16
#endif

/* Format can be unsigned, in which case replace S with U */
#if SAMPLE_SIZE == 32
typedef int32_t sample_t;
int format = AFMT_S32_NE; /* Signed 32bit native endian format */
#elif SAMPLE_SIZE == 16
typedef int16_t sample_t;
int format = AFMT_S16_NE; /* Signed 16bit native endian format */
#elif SAMPLE_SIZE == 8
typedef int8_t sample_t;
int format = AFMT_S8_NE; /* Signed 8bit native endian format */
#else
#error Unsupported sample format!
typedef int32_t sample_t;
int format = AFMT_S32_NE; /* Signed 32bit native endian format */
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
  int sampleCount;
  int sampleRate;
  int sampleSize;
  int nsamples;
  int mmap;
  oss_audioinfo audioInfo;
  audio_buf_info bufferInfo;
} config_t;


/* Error state is indicated by value=-1 in which case application exits
 * with error
 */
void checkError(const int value, const char *message)
{
  if (value == -1)
  {
    fprintf(stderr, "OSS error: %s %s\n", message, strerror(errno));
    exit(1);
  }
}



/* Calculate frag by giving it minimal size of buffer */
int size2frag(int x)
{
  int frag = 0;
  while ((1 << frag) < x) { ++frag; }
  return frag;
}


/* Split input buffer into channels. Input buffer is in interleaved format
 * which means if we have 2 channels (L and R), this is what the buffer of
 * 8 samples would contain: L,R,L,R,L,R,L,R. The result are two channels
 * containing: L,L,L,L and R,R,R,R.
 */
void ossSplit(config_t *config, sample_t *input, sample_t *output)
{
  int channel;
  int index;
  for (int i = 0; i < config->sampleCount; ++i)
  {
    channel = i % config->channels;
    index = i / config->channels;
    output[channel * index] = input[i];
  }
}


/* Convert channels into interleaved format and place it in output
 * buffer
 */
void ossMerge(config_t *config, sample_t *input, sample_t *output)
{
  for (int channel = 0; channel < config->channels; ++channel)
  {
    for (int index = 0; index < config->nsamples ; ++index)
    {
      output[index * config->channels + channel] = input[channel * index];
    }
  }
}

void ossInit(config_t *config)
{
  int error;
  int tmp;
  /* Open the device for read and write */
  config->fd = open(config->device, O_RDWR);
  checkError(config->fd, "open");

  /* Get device information */
  config->audioInfo.dev = -1;
  error = ioctl(config->fd, SNDCTL_ENGINEINFO, &(config->audioInfo));
  checkError(error, "SNDCTL_ENGINEINFO");
  printf("min_channels: %d\n", config->audioInfo.min_channels);
  printf("max_channels: %d\n", config->audioInfo.max_channels);
  printf("latency: %d\n", config->audioInfo.latency);
  printf("handle: %s\n", config->audioInfo.handle);
  if (config->audioInfo.min_rate > config->sampleRate || config->sampleRate > config->audioInfo.max_rate)
  {
    fprintf(stderr, "%s doesn't support chosen ", config->device);
    fprintf(stderr, "samplerate of %dHz!\n", config->sampleRate);
    exit(1);
  }
  if (config->channels < 1)
  {
    config->channels = config->audioInfo.max_channels;
  }

  /* If device is going to be used in mmap mode, disable all format
   * conversions. Official OSS documentation states error code should not be
   * checked. http://manuals.opensound.com/developer/mmap_test.c.html#LOC10
   */
  if (config->mmap)
  {
    tmp = 0;
    ioctl (config->fd, SNDCTL_DSP_COOKEDMODE, &tmp);
  }

  /* Set number of channels. If number of channels is chosen to the value
   * near the one wanted, save it in config
   */
  tmp = config->channels;
  error = ioctl(config->fd, SNDCTL_DSP_CHANNELS, &tmp);
  checkError(error, "SNDCTL_DSP_CHANNELS");
  if (tmp != config->channels) /* or check if tmp is close enough? */
  {
    fprintf(stderr, "%s doesn't support chosen ", config->device);
    fprintf(stderr, "channel count of %d", config->channels);
    fprintf(stderr, ", set to %d!\n", tmp);
  }
  config->channels = tmp;

  /* Set format, or bit size: 8, 16, 24 or 32 bit sample */
  tmp = config->format;
  error = ioctl(config->fd, SNDCTL_DSP_SETFMT, &tmp);
  checkError(error, "SNDCTL_DSP_SETFMT");
  if (tmp != config->format)
  {
    fprintf(stderr, "%s doesn't support chosen sample format!\n", config->device);
    exit(1);
  }

  /* Most common values for samplerate (in kHz): 44.1, 48, 88.2, 96 */
  tmp = config->sampleRate;
  error = ioctl(config->fd, SNDCTL_DSP_SPEED, &tmp);
  checkError(error, "SNDCTL_DSP_SPEED");

  /* Get and check device capabilities */
  error = ioctl(config->fd, SNDCTL_DSP_GETCAPS, &(config->audioInfo.caps));
  checkError(error, "SNDCTL_DSP_GETCAPS");
  if (!(config->audioInfo.caps & PCM_CAP_DUPLEX))
  {
    fprintf(stderr, "Device doesn't support full duplex!\n");
    exit(1);
  }
  if (config->mmap)
  {
    if (!(config->audioInfo.caps & PCM_CAP_TRIGGER))
    {
      fprintf(stderr, "Device doesn't support triggering!\n");
      exit(1);
    }
    if (!(config->audioInfo.caps & PCM_CAP_MMAP))
    {
      fprintf(stderr, "Device doesn't support mmap mode!\n");
      exit(1);
    }
  }

  /* If desired frag is smaller than minimum, based on number of channels
   * and format (size in bits: 8, 16, 24, 32), set that as frag. Buffer size
   * is 2^frag, but the real size of the buffer will be read when the
   * configuration of the device is successfull
   */
  int minFrag = size2frag(config->sampleSize * config->channels);
  if (config->frag < minFrag) { config->frag = minFrag; }

  /* Allocate buffer in fragments. Total buffer will be split in number
   * of fragments (2 by default)
   */
  if (config->bufferInfo.fragments < 0) { config->bufferInfo.fragments = 2; }
  tmp = ((config->bufferInfo.fragments) << 16) | config->frag;
  error = ioctl(config->fd, SNDCTL_DSP_SETFRAGMENT, &tmp);
  checkError(error, "SNDCTL_DSP_SETFRAGMENT");

  /* When all is set and ready to go, get the size of buffer */
  error = ioctl(config->fd, SNDCTL_DSP_GETOSPACE, &(config->bufferInfo));
  checkError(error, "SNDCTL_DSP_GETOSPACE");
  config->sampleCount = config->bufferInfo.bytes / config->sampleSize;
  config->nsamples =  config->sampleCount / config->channels;
}
