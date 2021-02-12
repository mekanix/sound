#include "ossinit.h"


int main()
{
  config_t config = {
    .device = "/dev/dsp",
    .channels = 2,
    .format = format,
    .frag = 10,
    .samplerate = 48000,
    .sampleSize = sizeof(sample_t),
    .bufferInfo.fragments = -1,
    .mmap = 0,
  };
  int error;

  /* Initialize device */
  ossInit(&config);

  /* When all is set and ready to go, get the size of buffer */
  error = ioctl(config.fd, SNDCTL_DSP_GETOSPACE, &(config.bufferInfo));
  checkError(error, "SNDCTL_DSP_GETOSPACE");
  int totalSamples = config.bufferInfo.bytes / config.sampleSize;
  config.nsamples =  totalSamples / config.channels;

  /* Allocate input and output buffers so that their size match fragSize */
  int8_t ibuf[config.bufferInfo.bytes];
  int8_t obuf[config.bufferInfo.bytes];
  printf(
    "bytes: %d, fragments: %d, fragsize: %d, fragstotal: %d, samples: %d\n",
    config.bufferInfo.bytes,
    config.bufferInfo.fragments,
    config.bufferInfo.fragsize,
    config.bufferInfo.fragstotal,
    totalSamples
  );

  /* Allocate buffer per channel */
  sample_t channels[config.channels][config.nsamples];

  /* Minimal engine: read input and copy it to the output */
  int i;
  int channel;
  int index;
  sample_t *input = (sample_t *)ibuf;
  sample_t *output = (sample_t *)obuf;
  while(1)
  {
    read(config.fd, ibuf, config.bufferInfo.bytes);
    /* Split input buffer into channels. Input buffer is in interleaved format
     * which means if we have 2 channels (L and R), this is what the buffer of
     * 8 samples would contain: L,R,L,R,L,R,L,R. The result are two channels
     * containing: L,L,L,L and R,R,R,R.
     */
    for (i = 0; i < totalSamples; ++i)
    {
      channel = i % config.channels;
      index = i / config.channels;
      channels[channel][index] = input[i];
    }

    /* Convert channels into interleaved format and place it in output
     * buffer
     */
    for (channel = 0; channel < config.channels; ++channel)
    {
      for (index = 0; index < config.nsamples ; ++index)
      {
        output[index * config.channels + channel] = channels[channel][index];
      }
    }
    write(config.fd, obuf, config.bufferInfo.bytes);
  }
  close(config.fd);
  return 0;
}
