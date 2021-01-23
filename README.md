# FreeBSD OSS

[Official OSS development howto](http://manuals.opensound.com/developer/DSP.html)

 * There is a software facing buffer (bs) and a hardware driver buffer (b)
 * The sizes can be seen with `cat /dev/sndstat` as [b:_:_:_] [bs:_:_:_]
 * OSS ioctl only concern software buffer fragments, not hardware
 * Software buffer fragments don't matter, there is no quantization going on
 * Hardware buffer fragments do matter for timing, but are inaccessible...

Not sure how virtual_oss handles them, but I think it is processed blockwise,
so try to match its "-s" parameter with Jack period and the block size of your
hardware - what is it BTW?

For USB the block size is according to buffer_ms sysctl, e.g. 2ms at 48kHz
gives me 0.002*48000=96 samples per block, all multiples of this work well as
Jack period.


# Previous mail

Actually it would be 2 fragments in the example that make up a total buffer
size of 2*1024. Thus my patch would divide it into 8 fragments of size.
*1024/8 = 256 samples instead.

The problem lies not in the number of bytes written, but in the timing. Jack
insists on writing / reading a certain number of samples at a time, one period
per cycle. It is bound to do so in a fixed time frame, because Jack clients
need input / output at a defined latency. OTOH, OSS driver also insists on
reading / writing a certain number of samples at a time, one fragment full of
samples. It is bound to do so in a fixed time frame, to avoid under- and
overruns in communication with the hardware.

Now both Jack and OSS have to meet each others timing constraints when reading
/ writing the OSS buffer. The idea of a total buffer size that holds 2*period
samples is to give some slack and allow Jack to be about one period late. I
call this the jitter tolerance. But as shown in the example above, the jitter
tolerance may be much less if there is a slight mismatch between the period
and the samples per fragment.

Jitter tolerance gets better if we can make either the period or the samples
per fragment considerably smaller than the other. In our case that means we
divide the total buffer size into smaller fragments, keeping overall latency
at the same level.


# Compiling

```
cc oss.c -o oss
```
