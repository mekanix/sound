#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/soundcard.h>
#include <unistd.h>


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


int main()
{
  config_t config = {
    .device = "/dev/dsp",
    .channels = 2,
    .format = AFMT_S32_NE, /* Signed 32bit native endian format */
    .frag = 10,
    .samplerate = 48000,
  };
  int devcaps;
  int error;
  int tmp;
  int bufferSize = 1024;
  int formatSize = sizeof(int32_t);
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
   * and format (size in bits: 8, 16, 24, 32), set that as frag
   */
  int minFrag = size2frag(formatSize * config.channels);
  if (config.frag < minFrag) { config.frag = minFrag; }
  config.frag = (1 << 16) | config.frag;
  tmp = config.frag;
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
  return 0;
}
