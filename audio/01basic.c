#include "ossinit.h"


int main()
{
  config_t config = {
    .device = "/dev/dsp",
    .channels = -1,
    .format = format,
    .frag = 10,
    .sampleRate = 48000,
    .sampleSize = sizeof(sample_t),
    .bufferInfo.fragments = -1,
    .mmap = 0,
  };

  /* Initialize device */
  ossInit(&config);

  /* Allocate input and output buffers so that their size match fragSize */
  int bytes = config.bufferInfo.bytes;
  int8_t ibuf[bytes];
  int8_t obuf[bytes];
  sample_t *channels = (sample_t *)malloc(bytes);
  printf(
    "bytes: %d, fragments: %d, fragsize: %d, fragstotal: %d, samples: %d\n",
    bytes,
    config.bufferInfo.fragments,
    config.bufferInfo.fragsize,
    config.bufferInfo.fragstotal,
    config.sampleCount
  );

  /* Minimal engine: read input and copy it to the output */
  while(1)
  {
    read(config.fd, ibuf, bytes);
    ossSplit(&config, (sample_t *)ibuf, channels);
    ossMerge(&config, channels, (sample_t *)obuf);
    write(config.fd, obuf, bytes);
  }

  /* Cleanup */
  free(channels);
  close(config.fd);
  return 0;
}
