# FreeBSD OSS MIDI

MIDI is the simplest of OSS APIs. Most MIDI events are 4 bytes long messages
that contain type (note on/off, controller, ...) and channel as one byte,
note or controller number depending on the type, and pitch of note or value
of controller. So, the stream of bytes starts with `ttttcccc` as first byte
where `t` stands for type and `c` for channel of the event. Assuming it's a
note event, second byte is not pitch and third byte is velocity. All values
are unsigned. For controller type, second byte is controller number and third
byte is controller value.
