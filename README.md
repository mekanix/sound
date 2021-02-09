# FreeBSD OSS

The complete OSS API is divided in 3 chapters:

* [Audio](audio)
* [Midi](midi)
* [Mixer](mixer)

The common idea for all of them is using standard functions to operate the
device:

* Open /dev/*
* Get info and configure device using ioctl
* Close the device

For audio and MIDI, there will be a loop to do the following:

* Read samples
* If needed, convert them (eg, interleaved vs non-interleaved audio)
* Processing and FX
* If needed, convert format back to OSS native
* Write samples
