#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/soundcard.h>
#include <unistd.h>


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
  int channels = 2;
  int devcaps;
  int error;
  int fd;
  int format = AFMT_S32_NE;
  int frag = 10;
  int fragSize;
  int samplerate = 48000;
  int tmp;
  int bufferSize = 1024;
  int formatSize = sizeof(int32_t);
  char *deviceName = "/dev/dsp";
  oss_audioinfo ai;

  fd = open(deviceName, O_RDWR, 0);
  checkError(fd, "open");

  ai.dev = -1;
  error = ioctl(fd, SNDCTL_ENGINEINFO, &ai);
  checkError(error, "SNDCTL_ENGINEINFO");
  printf("min_channels: %d\n", ai.min_channels);
  printf("max_channels: %d\n", ai.max_channels);
  printf("latency: %d\n", ai.latency);
  printf("handle: %s\n", ai.handle);
  if (ai.min_rate > samplerate || samplerate > ai.max_rate)
  {
    fprintf(stderr, "%s doesn't support chosen ", deviceName);
    fprintf(stderr, "samplerate of %dHz!\n", samplerate);
    exit(1);
  }

  error = ioctl(fd, SNDCTL_DSP_GETCAPS, &devcaps);
  checkError(error, "SNDCTL_DSP_GETCAPS");

  tmp = channels;
  error = ioctl(fd, SNDCTL_DSP_CHANNELS, &tmp);
  checkError(error, "SNDCTL_DSP_CHANNELS");
  if (tmp != channels)
  {
    fprintf(stderr, "%s doesn't support chosen ", deviceName);
    fprintf(stderr, "channel count of %d", channels);
    fprintf(stderr, ", set to %d!\n", tmp);
  }
  channels = tmp;

  int minFrag = size2frag(formatSize * channels);
  if (frag < minFrag)
  {
    int fsize = formatSize * 8;
    fprintf(stderr, "Minimal fragmet size for %d channels", channels);
    fprintf(stderr, " and %dbit sample size is %d!\n", fsize, minFrag);
    exit(1);
  }
  frag = tmp;
  error = ioctl(fd, SNDCTL_DSP_SETFRAGMENT, &tmp);
  checkError(error, "SNDCTL_DSP_SETFRAGMENT");
  if (tmp != frag) { fprintf(stderr, "Fragment size set to %d!\n", tmp); }
  frag = tmp;

  tmp = format;
  error = ioctl(fd, SNDCTL_DSP_SETFMT, &tmp);
  checkError(error, "SNDCTL_DSP_SETFMT");
  if (tmp != format)
  {
    fprintf(stderr, "%s doesn't support chosen sample format!\n", deviceName);
    exit(1);
  }
  format = tmp;

  tmp = samplerate;
  error = ioctl(fd, SNDCTL_DSP_SPEED, &tmp);
  checkError(error, "SNDCTL_DSP_SPEED");

  error = ioctl(fd, SNDCTL_DSP_GETBLKSIZE, &fragSize);
  checkError(error, "SNDCTL_DSP_GETBLKSIZE");
  return 0;
}
