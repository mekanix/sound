#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/soundcard.h>
#include <unistd.h>


typedef struct config
{
  char *device;
  int channels;
  int fd;
  int format;
  int frag;
  int fragSize;
  int samplerate;
} config_t;


static void checkError(const int value, const char *message)
{
  if (value == -1)
  {
    fprintf(stderr, "OSS error: %s %s\n", message, strerror(errno));
    exit(1);
  }
}


static int size2frag(int x)
{
  int frag = 0;
  while ((1 << frag) < x) { ++frag; }
  return frag;
}


int main()
{
  config_t config = {
    .device = "/dev/dsp",
    .channels = 2,
    .format = AFMT_S32_NE,
    .frag = 10,
    .samplerate = 48000,
  };
  int devcaps;
  int error;
  int tmp;
  int bufferSize = 1024;
  int formatSize = sizeof(int32_t);
  oss_audioinfo ai;

  config.fd = open(config.device, O_RDWR, 0);
  checkError(config.fd, "open");

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

  int minFrag = size2frag(formatSize * config.channels);
  if (config.frag < minFrag)
  {
    config.frag = minFrag;
    int fsize = formatSize * 8;
    fprintf(stderr, "Minimal fragmet size for %d channels", config.channels);
    fprintf(stderr, " and %dbit sample size is %d!\n", fsize, minFrag);
    exit(1);
  }
  config.frag = (1 << 16) | config.frag;
  tmp = config.frag;
  error = ioctl(config.fd, SNDCTL_DSP_SETFRAGMENT, &tmp);
  checkError(error, "SNDCTL_DSP_SETFRAGMENT");

  tmp = config.format;
  error = ioctl(config.fd, SNDCTL_DSP_SETFMT, &tmp);
  checkError(error, "SNDCTL_DSP_SETFMT");
  if (tmp != config.format)
  {
    fprintf(stderr, "%s doesn't support chosen sample format!\n", config.device);
    exit(1);
  }

  tmp = config.samplerate;
  error = ioctl(config.fd, SNDCTL_DSP_SPEED, &tmp);
  checkError(error, "SNDCTL_DSP_SPEED");

  error = ioctl(config.fd, SNDCTL_DSP_GETBLKSIZE, &config.fragSize);
  checkError(error, "SNDCTL_DSP_GETBLKSIZE");
  return 0;
}
