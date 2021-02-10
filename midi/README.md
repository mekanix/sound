# FreeBSD OSS MIDI

MIDI is the simplest of OSS APIs. Most MIDI events are 4 bytes long messages
that contain type of message (note on/off, controller, ...) and channel as one
byte, note or controller number depending on the type, and pitch of note or
value of controller.
