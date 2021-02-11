# FreeBSD OSS MIDI

## Introduction

MIDI is the simplest of OSS APIs. Most MIDI events are 4 bytes long messages
that contain type (note on/off, controller, ...) and channel as one byte,
note or controller number depending on the type, and pitch of note or value
of controller. So, the stream of bytes starts with `ttttcccc` as first byte
where `t` stands for type and `c` for channel of the event. Assuming it's a
note event, second byte is not pitch and third byte is velocity. All values
are unsigned. For controller type, second byte is controller number and third
byte is controller value.

## MIDI Files

Although file parsing/writing is not part of this documentation, there are few
points that you need to keep in mind while developing MIDI application. All
MIDI files in SMF format (most common) are big endian so you need to convert
them to your native endines. For x86 that means little endian. For more
information take a look at
[Standard MIDI File Structure](http://www.ccarh.org/courses/253/handout/smf/).
In any case, it's recommended to use MIDI file parsing library instead of
writing your own code.

## Compiling

```
cc <example>.c -o <example>
```
